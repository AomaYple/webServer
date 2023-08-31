#include "EventLoop.h"

#include "../base/Completion.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../http/Http.h"
#include "../log/Log.h"

#include <cstring>

using namespace std;

EventLoop::EventLoop()
    : userRing{[](source_location sourceLocation = source_location::current()) {
          if (EventLoop::instance)
              throw Exception{Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                           LogLevel::Fatal, "can not create more than one event loop")};
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
                  throw Exception{Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                               LogLevel::Fatal, "no available cpu")};

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

            switch (userData.eventType) {
                case EventType::Accept:
                    this->acceptEvent(completion.getResult(), completion.getFlags());

                    break;
                case EventType::Timeout:
                    this->timeoutEvent(completion.getResult());

                    break;
                case EventType::Receive:
                    this->receiveEvent(completion.getResult(), completion.getFlags(), userData.fileDescriptor);

                    break;
                case EventType::Send:
                    this->sendEvent(completion.getResult(), completion.getFlags(), userData.fileDescriptor);

                    break;
                case EventType::Cancel:
                    EventLoop::cancelEvent(completion.getResult());

                    break;
                case EventType::Close:
                    EventLoop::closeEvent(completion.getResult());

                    break;
            }
        })};

        this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
    }
}

auto EventLoop::acceptEvent(int result, unsigned int flags, source_location sourceLocation) -> void {
    if (result >= 0) {
        Client client{static_cast<unsigned int>(result), 60, this->userRing};

        client.receive(this->bufferRing.getId());

        this->timer.add(std::move(client));
    } else
        throw Exception{Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                     LogLevel::Error, "accept error: " + string{strerror(abs(result))})};

    if (!(flags & IORING_CQE_F_MORE)) {
        this->server.accept();

        Log::produce(Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, LogLevel::Error,
                                  "can not accept"));
    }
}

auto EventLoop::timeoutEvent(int result, source_location sourceLocation) -> void {
    if (result == sizeof(unsigned long)) {
        this->timer.clearTimeout();

        this->timer.startTiming();
    } else
        throw Exception{Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                     LogLevel::Fatal, "timeout error: " + string{strerror(abs(result))})};
}

auto EventLoop::receiveEvent(int result, unsigned int flags, unsigned int fileDescriptor,
                             source_location sourceLocation) -> void {
    if (result <= 0) {
        Log::produce(Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, LogLevel::Warn,
                                  "receive error: " + string{strerror(abs(result))}));

        if (abs(result) == ECANCELED) return;
    }

    if (result > 0) {
        Client &client{this->timer.getClient(fileDescriptor)};

        if (!(flags & IORING_CQE_F_MORE)) client.receive(this->bufferRing.getId());

        client.writeReceivedData(this->bufferRing.getData(flags >> IORING_CQE_BUFFER_SHIFT, result));

        if (!(flags & IORING_CQE_F_SOCK_NONEMPTY)) {
            client.send(Http::parse(client.readReceivedData(), this->database));

            this->timer.update(fileDescriptor);
        }
    } else
        this->timer.remove(fileDescriptor);
}

auto EventLoop::sendEvent(int result, unsigned int flags, unsigned int fileDescriptor, source_location sourceLocation)
        -> void {
    if ((result == 0 && !(flags & IORING_CQE_F_NOTIF)) || result < 0) {
        Log::produce(Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, LogLevel::Error,
                                  "send error: " + string{strerror(abs(result))}));

        if (std::abs(result) == ECANCELED) return;

        this->timer.remove(fileDescriptor);
    }
}

auto EventLoop::cancelEvent(int result, source_location sourceLocation) -> void {
    Log::produce(Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, LogLevel::Error,
                              "cancel error: " + string{strerror(abs(result))}));
}

auto EventLoop::closeEvent(int result, source_location sourceLocation) -> void {
    Log::produce(Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, LogLevel::Error,
                              "close error: " + string{strerror(abs(result))}));
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
