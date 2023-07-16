#include "EventLoop.h"

#include <cstring>

#include "Completion.h"
#include "Event.h"
#include "Http.h"
#include "Log.h"

using std::jthread, std::mutex, std::lock_guard;
using std::runtime_error;
using std::shared_ptr, std::make_shared;
using std::source_location;
using std::string, std::to_string, std::vector;
using std::ranges::find_if;

constexpr unsigned int ringEntries{128};
constexpr unsigned short bufferRingEntries{128}, bufferRingId{0}, serverPort{9999};
constexpr unsigned long bufferRingBufferSize{1024};

constinit thread_local bool EventLoop::instance{false};
constinit mutex EventLoop::lock{};
vector<int> EventLoop::values{vector<int>(jthread::hardware_concurrency(), -1)};

EventLoop::EventLoop()
    : userRing{[] {
          if (EventLoop::instance) throw runtime_error{"eventLoop instance already exists"};
          EventLoop::instance = true;

          io_uring_params params{};

          params.flags = IORING_SETUP_SUBMIT_ALL | IORING_SETUP_CLAMP | IORING_SETUP_COOP_TASKRUN |
                         IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

          shared_ptr<UserRing> tempUserRing;

          {
              lock_guard lockGuard{EventLoop::lock};

              auto result{find_if(EventLoop::values, [](int value) { return value != -1; })};
              if (result != EventLoop::values.end()) {
                  params.wq_fd = *result;
                  params.flags |= IORING_SETUP_ATTACH_WQ;
              }

              tempUserRing = make_shared<UserRing>(ringEntries, params);

              result = find_if(EventLoop::values, [](int value) { return value == -1; });
              if (result != EventLoop::values.end()) *result = tempUserRing->getSelfFileDescriptor();
              else
                  throw runtime_error("eventLoop instance number is too large");

              tempUserRing->registerCpu(static_cast<int>(std::distance(EventLoop::values.begin(), result)));
          }

          tempUserRing->registerSelfFileDescriptor();

          tempUserRing->registerFileDescriptors(getFileDescriptorLimit());

          return tempUserRing;
      }()},
      bufferRing{bufferRingEntries, bufferRingBufferSize, bufferRingId, this->userRing},
      server{serverPort, this->userRing} {}

auto EventLoop::loop() -> void {
    this->server.accept();

    this->timer.start(Submission{this->userRing->getSubmission()});

    while (true) {
        this->userRing->submitWait(1);

        unsigned int completionCount{this->userRing->forEachCompletion([this](io_uring_cqe *cqe) -> void {
            Completion completion{cqe};

            int result{completion.getResult()};
            unsigned long long userData{completion.getUserData()};
            unsigned int flags{completion.getFlags()};

            Event event{reinterpret_cast<Event &>(userData)};

            switch (event.type) {
                case Type::ACCEPT:
                    this->handleAccept(result, flags);

                    break;
                case Type::TIMEOUT:
                    this->handleTimeout(result);

                    break;
                case Type::RECEIVE:
                    this->handleReceive(result, event.socket, flags);

                    break;
                case Type::SEND:
                    this->handleSend(result, event.socket, flags);

                    break;
                case Type::CANCEL:
                    EventLoop::handleCancel(result, event.socket);

                    break;
                case Type::CLOSE:
                    EventLoop::handleClose(result, event.socket);

                    break;
            }
        })};

        this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
    }
}

EventLoop::~EventLoop() {
    EventLoop::instance = false;

    int fileDescriptor{this->userRing->getSelfFileDescriptor()};

    lock_guard lockGuard{EventLoop::lock};

    auto result{find_if(EventLoop::values, [fileDescriptor](int value) { return value == fileDescriptor; })};

    if (result != EventLoop::values.end()) *result = -1;
    else
        Log::produce(source_location::current(), Level::ERROR, "eventLoop can not find self file descriptor");
}

auto EventLoop::handleAccept(int result, unsigned int flags, source_location sourceLocation) -> void {
    if (result >= 0) {
        Client client{result, 30, this->userRing};

        client.receive(this->bufferRing.getId());

        this->timer.add(std::move(client));
    } else
        Log::produce(sourceLocation, Level::WARN, "server accept error: " + string{std::strerror(std::abs(result))});

    if (!(flags & IORING_CQE_F_MORE)) this->server.accept();
}

auto EventLoop::handleTimeout(int result) -> void {
    if (result == sizeof(unsigned long)) {
        this->timer.clearTimeout();

        this->timer.start(Submission{this->userRing->getSubmission()});
    } else
        throw runtime_error("timer timing error: " + string{std::strerror(std::abs(result))});
}

auto EventLoop::handleReceive(int result, int socket, unsigned int flags, source_location sourceLocation) -> void {
    if (this->timer.exist(socket) && result > 0) {
        Client client{this->timer.pop(socket)};

        client.writeReceivedData(this->bufferRing.getData(flags >> IORING_CQE_BUFFER_SHIFT, result));

        if (!(flags & IORING_CQE_F_SOCK_NONEMPTY)) client.send(Http::parse(client.readReceivedData()));

        if (!(flags & IORING_CQE_F_MORE)) client.receive(this->bufferRing.getId());

        this->timer.add(std::move(client));
    } else if (std::abs(result) != ECANCELED) {
        this->timer.pop(socket);

        if (result < 0)
            Log::produce(sourceLocation, Level::WARN,
                         "client receive error: " + string{std::strerror(std::abs(result))});
    }
}

auto EventLoop::handleSend(int result, int socket, unsigned int flags, source_location sourceLocation) -> void {
    if (this->timer.exist(socket) && result == 0 && (flags & IORING_CQE_F_NOTIF)) {
        Client client{this->timer.pop(socket)};

        this->timer.add(std::move(client));
    } else if (result < 0 && std::abs(result) != ECANCELED) {
        this->timer.pop(socket);

        Log::produce(sourceLocation, Level::WARN, "client send error: " + string{std::strerror(std::abs(result))});
    }
}

auto EventLoop::handleCancel(int result, int socket, source_location sourceLocation) noexcept -> void {
    Log::produce(sourceLocation, Level::WARN,
                 "socket " + to_string(socket) + " cancel error: " + string{std::strerror(std::abs(result))});
}

auto EventLoop::handleClose(int result, int socket, source_location sourceLocation) noexcept -> void {
    Log::produce(sourceLocation, Level::WARN,
                 "socket " + to_string(socket) + " close error: " + string{std::strerror(std::abs(result))});
}
