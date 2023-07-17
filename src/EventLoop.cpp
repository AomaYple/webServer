#include "EventLoop.h"

#include "Completion.h"
#include "Event.h"
#include "Log.h"

using std::jthread, std::mutex, std::lock_guard;
using std::out_of_range;
using std::shared_ptr, std::make_shared;
using std::source_location;
using std::vector;
using std::ranges::find_if;

constexpr unsigned int ringEntries{128};
constexpr unsigned short bufferRingEntries{128}, bufferRingId{0}, serverPort{9999};
constexpr unsigned long bufferRingBufferSize{1024};

constinit thread_local bool EventLoop::instance{false};
constinit mutex EventLoop::lock{};
vector<int> EventLoop::cpus{vector<int>(jthread::hardware_concurrency(), -1)};

EventLoop::EventLoop()
    : userRing{[] {
          if (EventLoop::instance) throw out_of_range{"eventLoop instance already exists"};
          EventLoop::instance = true;

          io_uring_params params{};

          params.flags = IORING_SETUP_SUBMIT_ALL | IORING_SETUP_CLAMP | IORING_SETUP_COOP_TASKRUN |
                         IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

          shared_ptr<UserRing> tempUserRing;

          {
              lock_guard lockGuard{EventLoop::lock};

              auto result{find_if(EventLoop::cpus, [](int cpu) { return cpu != -1; })};
              if (result != EventLoop::cpus.end()) {
                  params.wq_fd = *result;
                  params.flags |= IORING_SETUP_ATTACH_WQ;
              }

              tempUserRing = make_shared<UserRing>(ringEntries, params);

              result = find_if(EventLoop::cpus, [](int value) { return value == -1; });
              if (result != EventLoop::cpus.end()) *result = tempUserRing->getSelfFileDescriptor();
              else
                  throw out_of_range("eventLoop instance count is too large");

              tempUserRing->registerCpu(static_cast<unsigned short>(std::distance(EventLoop::cpus.begin(), result)));
          }

          tempUserRing->registerSelfFileDescriptor();

          tempUserRing->registerFileDescriptors(getFileDescriptorLimit());

          return tempUserRing;
      }()},
      bufferRing{bufferRingEntries, bufferRingBufferSize, bufferRingId, this->userRing},
      server{serverPort, this->userRing} {}

auto EventLoop::loop() -> void {
    this->server.accept();

    this->timer.start(this->userRing->getSubmission());

    while (true) {
        this->userRing->submitWait(1);

        unsigned int completionCount{this->userRing->forEachCompletion([this](io_uring_cqe *cqe) -> void {
            Completion completion{cqe};

            unsigned long long completionUserData{completion.getUserData()};

            UserData userData{reinterpret_cast<UserData &>(completionUserData)};

            Event *event{Event::create(userData.type)};

            event->handle(completion.getResult(), userData.fileDescriptor, completion.getFlags(), this->userRing,
                          this->bufferRing, this->server, this->timer, source_location::current());

            delete event;
        })};

        this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
    }
}

EventLoop::~EventLoop() {
    EventLoop::instance = false;
    int fileDescriptor{this->userRing->getSelfFileDescriptor()};

    lock_guard lockGuard{EventLoop::lock};

    auto result{find_if(EventLoop::cpus, [fileDescriptor](int value) { return value == fileDescriptor; })};

    if (result != EventLoop::cpus.end()) *result = -1;
    else
        Log::produce(source_location::current(), Level::ERROR, "eventLoop can not find userRing file descriptor");
}
