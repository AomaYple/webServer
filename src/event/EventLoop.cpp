#include "EventLoop.h"

#include "../base/Completion.h"
#include "../base/Submission.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../log/Log.h"
#include "../log/message.h"
#include "Event.h"

using std::array, std::mutex, std::lock_guard, std::source_location, std::unique_ptr, std::shared_ptr, std::vector;
using std::ranges::find_if, std::make_shared, std::this_thread::get_id;
;
using std::jthread, std::chrono::system_clock;

EventLoop::EventLoop()
    : userRing{[](source_location sourceLocation = source_location::current()) {
          if (EventLoop::instance) throw Exception{"only one instance"};
          EventLoop::instance = true;

          io_uring_params params{};

          params.flags = IORING_SETUP_SUBMIT_ALL | IORING_SETUP_CLAMP | IORING_SETUP_COOP_TASKRUN |
                         IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

          shared_ptr<UserRing> tempUserRing;

          {
              const lock_guard lockGuard{EventLoop::lock};

              auto result{find_if(EventLoop::cpus, [](int cpu) { return cpu != -1; })};
              if (result != EventLoop::cpus.end()) {
                  params.wq_fd = *result;
                  params.flags |= IORING_SETUP_ATTACH_WQ;
              }

              tempUserRing = make_shared<UserRing>(EventLoop::ringEntries, params);

              result = find_if(EventLoop::cpus, [](int value) { return value == -1; });
              if (result != EventLoop::cpus.end()) *result = tempUserRing->getSelfFileDescriptor();
              else
                  throw Exception{"no available cpu"};

              tempUserRing->registerCpu(std::distance(EventLoop::cpus.begin(), result));
          }

          tempUserRing->registerSelfFileDescriptor();

          tempUserRing->registerFileDescriptors(UserRing::getFileDescriptorLimit());

          tempUserRing->allocateFileDescriptorRange(2, UserRing::getFileDescriptorLimit() - 2);

          return tempUserRing;
      }()},
      bufferRing{EventLoop::bufferRingEntries, EventLoop::bufferRingBufferSize, EventLoop::bufferRingId,
                 this->userRing},
      server{EventLoop::port, this->userRing}, database{{}, "AomaYple", "38820233", "WebServer", 0, {}, 0},
      timer(this->userRing) {
    const array<int, 2> fileDescriptors{this->server.getFileDescriptor(), this->timer.getFileDescriptor()};
    this->userRing->updateFileDescriptors(0, fileDescriptors);

    this->server.setFileDescriptor(0);
    this->timer.setFileDescriptor(1);
}

EventLoop::EventLoop(EventLoop &&other) noexcept
    : userRing{std::move(other.userRing)}, bufferRing{std::move(other.bufferRing)}, server{std::move(other.server)},
      timer{std::move(other.timer)}, database{std::move(other.database)} {}

auto EventLoop::loop() -> void {
    this->server.accept();

    this->timer.startTiming();

    while (true) {
        this->userRing->submitWait(1);

        int completionCount{this->userRing->forEachCompletion([this](io_uring_cqe *cqe) {
            const Completion completion{cqe};

            const __u64 completionUserData{completion.getUserData()};

            const UserData userData{reinterpret_cast<const UserData &>(completionUserData)};

            const unique_ptr<Event> event{Event::create(userData.type)};

            event->handle(completion.getResult(), userData.fileDescriptor, completion.getFlags(), this->userRing,
                          this->bufferRing, this->server, this->timer, this->database, source_location::current());
        })};

        this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
    }
}

EventLoop::~EventLoop() {
    EventLoop::instance = false;
    const int fileDescriptor{this->userRing->getSelfFileDescriptor()};

    const lock_guard lockGuard{EventLoop::lock};

    auto const result{find_if(EventLoop::cpus, [fileDescriptor](int value) { return value == fileDescriptor; })};

    if (result != EventLoop::cpus.end()) *result = -1;
    else
        Log::produce(message::combine(system_clock::now(), get_id(), source_location::current(), Level::FATAL,
                                      "can not find file descriptor"));
}

constinit thread_local bool EventLoop::instance{false};
constinit mutex EventLoop::lock;
vector<int> EventLoop::cpus{vector<int>(jthread::hardware_concurrency(), -1)};
