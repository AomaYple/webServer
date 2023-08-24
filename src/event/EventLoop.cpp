#include "EventLoop.h"

#include "../base/Completion.h"
#include "../base/Submission.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../log/Log.h"
#include "Event.h"

using namespace std;

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

              auto result{ranges::find_if(EventLoop::cpus, [](int cpu) { return cpu != -1; })};
              if (result != EventLoop::cpus.end()) {
                  params.wq_fd = *result;
                  params.flags |= IORING_SETUP_ATTACH_WQ;
              }

              tempUserRing = make_shared<UserRing>(EventLoop::ringEntries, params);

              result = ranges::find_if(EventLoop::cpus, [](int value) { return value == -1; });
              if (result != EventLoop::cpus.end()) *result = static_cast<int>(tempUserRing->getSelfFileDescriptor());
              else
                  throw Exception{"no available cpu"};

              tempUserRing->registerCpu(std::distance(EventLoop::cpus.begin(), result));
          }

          return tempUserRing;
      }()},
      bufferRing{EventLoop::bufferRingEntries, EventLoop::bufferRingBufferSize, EventLoop::bufferRingId,
                 this->userRing},
      server{0, this->userRing}, database{{}, "AomaYple", "38820233", "webServer", 0, {}, 0}, timer(1, this->userRing) {
    this->userRing->registerSelfFileDescriptor();

    this->userRing->registerSparseFileDescriptors(UserRing::getFileDescriptorLimit());

    this->userRing->allocateFileDescriptorRange(2, UserRing::getFileDescriptorLimit() - 2);

    const array<unsigned int, 2> fileDescriptors{Server::create(EventLoop::port), Timer::create()};

    this->userRing->updateFileDescriptors(0, fileDescriptors);
}

EventLoop::EventLoop(EventLoop &&other) noexcept
    : userRing{std::move(other.userRing)}, bufferRing{std::move(other.bufferRing)}, server{std::move(other.server)},
      timer{std::move(other.timer)}, database{std::move(other.database)} {}

auto EventLoop::loop() -> void {
    this->server.accept();

    this->timer.startTiming();

    while (true) {
        this->userRing->submitWait(1);

        unsigned int completionCount{this->userRing->forEachCompletion([this](io_uring_cqe *cqe) {
            const Completion completion{cqe};

            const unsigned long completionUserData{completion.getUserData()};

            const UserData userData{reinterpret_cast<const UserData &>(completionUserData)};

            const unique_ptr<Event> event{Event::create(userData.eventType)};

            event->handle(completion.getResult(), userData.fileDescriptor, completion.getFlags(), this->userRing,
                          this->bufferRing, this->server, this->timer, this->database, source_location::current());
        })};

        this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
    }
}

EventLoop::~EventLoop() {
    EventLoop::instance = false;
    const unsigned int fileDescriptor{this->userRing->getSelfFileDescriptor()};

    const lock_guard lockGuard{EventLoop::lock};

    const auto result{
            ranges::find_if(EventLoop::cpus, [fileDescriptor](int value) { return value == fileDescriptor; })};

    if (result != EventLoop::cpus.end()) *result = -1;
    else
        Log::produce(Log::combine(chrono::system_clock::now(), this_thread::get_id(), source_location::current(),
                                  LogLevel::Fatal, "can not find file descriptor"));
}

constinit thread_local bool EventLoop::instance{false};
constinit mutex EventLoop::lock;
vector<int> EventLoop::cpus{vector<int>(jthread::hardware_concurrency(), -1)};
