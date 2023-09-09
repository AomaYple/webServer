#include "EventLoop.h"

#include "../base/Completion.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../http/Http.h"
#include "../log/Log.h"
#include "Connection.h"

#include <algorithm>
#include <cstring>

using namespace std;

EventLoop::EventLoop()
    : userRing{[](source_location sourceLocation = source_location::current()) {
          if (EventLoop::instance)
              throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                             sourceLocation, "EventLoop instance already exists")};
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
                  throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                                 sourceLocation, "no available cpu")};

              tempUserRing->registerCpu(std::distance(EventLoop::cpus.begin(), result));
          }

          return tempUserRing;
      }()},
      bufferRing{EventLoop::bufferRingEntries, EventLoop::bufferRingBufferSize, EventLoop::bufferRingId,
                 this->userRing},
      server{0}, timer{1}, database{{}, "AomaYple", "38820233", "webServer", 0, {}, 0}, serverGenerator{nullptr},
      timerGenerator{nullptr} {
    this->userRing->registerSelfFileDescriptor();

    this->userRing->registerSparseFileDescriptors(UserRing::getFileDescriptorLimit());

    this->userRing->allocateFileDescriptorRange(2, UserRing::getFileDescriptorLimit() - 2);

    const array<int, 2> fileDescriptors{static_cast<int>(Server::create(EventLoop::port)),
                                        static_cast<int>(Timer::create())};

    this->userRing->updateFileDescriptors(0, fileDescriptors);
}

EventLoop::EventLoop(EventLoop &&other) noexcept
    : userRing{std::move(other.userRing)}, bufferRing{std::move(other.bufferRing)}, server{std::move(other.server)},
      timer{std::move(other.timer)}, database{std::move(other.database)},
      serverGenerator{std::move(other.serverGenerator)}, timerGenerator{std::move(other.timerGenerator)},
      connections{std::move(other.connections)} {}

auto EventLoop::loop() -> void {
    this->server.startAccept(this->userRing->getSqe());
    this->serverGenerator = this->accept();
    this->serverGenerator.resume();

    this->timerGenerator = this->timing();
    this->timerGenerator.resume();

    while (true) {
        this->userRing->submitWait(1);

        unsigned int completionCount{this->userRing->forEachCompletion([this](io_uring_cqe *cqe) {
            const Completion completion{cqe};

            const unsigned long completionUserData{completion.getUserData()};
            const UserData userData{reinterpret_cast<const UserData &>(completionUserData)};

            const pair<int, unsigned int> result{completion.getResult(), completion.getFlags()};

            switch (userData.eventType) {
                case EventType::Accept:
                    this->server.setResult(result);
                    this->serverGenerator.resume();

                    this->serverGenerator = this->accept();
                    this->serverGenerator.resume();

                    break;
                case EventType::Timeout:
                    this->timer.setResult(result);
                    this->timerGenerator.resume();

                    this->timerGenerator = this->timing();
                    this->timerGenerator.resume();

                    break;
                case EventType::Receive:
                    if (result.first >= 0 || abs(result.first) != ECANCELED) {
                        Connection &connection{this->connections.at(userData.fileDescriptor)};

                        connection.client.setResult(result);
                        connection.generator.resume();
                    }

                    break;
                case EventType::Send:
                    if (!(result.second & IORING_CQE_F_MORE)) {
                        Connection &connection{this->connections.at(userData.fileDescriptor)};

                        connection.client.setResult(result);
                        connection.generator.resume();
                    }

                    break;
                case EventType::Cancel: {
                    Connection &connection{this->connections.at(userData.fileDescriptor)};

                    connection.client.setResult(result);
                    connection.generator.resume();

                    break;
                }
                case EventType::Close:
                    Connection &connection{this->connections.at(userData.fileDescriptor)};

                    connection.client.setResult(result);
                    connection.generator.resume();

                    this->connections.erase(userData.fileDescriptor);

                    break;
            }
        })};

        this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
    }
}

auto EventLoop::accept(source_location sourceLocation) -> Generator {
    const pair<int, unsigned int> result{co_await this->server.accept()};

    if (!(result.second & IORING_CQE_F_MORE))
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, "can not accept client")};

    if (result.first >= 0) {
        Client client{static_cast<unsigned int>(result.first), 60};

        client.startReceive(this->userRing->getSqe(), this->bufferRing.getId());

        this->timer.add(result.first, client.getTimeout());

        this->connections.emplace(result.first, Connection{std::move(client)});

        Connection &connection{this->connections.at(result.first)};

        connection.generator = this->receive(connection);
        connection.generator.resume();
    } else
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, strerror(abs(result.first)))};
}

auto EventLoop::timing(source_location sourceLocation) -> Generator {
    const pair<int, unsigned int> result{co_await this->timer.timing(this->userRing->getSqe())};

    if (result.first == sizeof(unsigned long))
        ranges::for_each(this->timer.clearTimeout(), [this](unsigned int fileDescriptor) {
            Connection &connection{this->connections.at(fileDescriptor)};

            connection.generator = this->cancel(connection);
            connection.generator.resume();
        });
    else
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, strerror(abs(result.first)))};
}

auto EventLoop::receive(Connection &connection, source_location sourceLocation) -> Generator {
    const pair<int, unsigned int> result{co_await connection.client.receive()};

    if (!(result.second & IORING_CQE_F_MORE)) {
        this->timer.remove(connection.client.getFileDescriptorIndex());

        Log::produce(Log::formatLog(Log::Level::Error, chrono::system_clock::now(), this_thread::get_id(),
                                    sourceLocation, "can not receiveGenerator request"));

        connection.generator = Generator{this->cancel(connection)};
        connection.generator.resume();
    } else if (result.first > 0) {
        const vector<byte> request{this->bufferRing.getData(result.second >> IORING_CQE_BUFFER_SHIFT, result.first)};
        connection.buffer.insert(connection.buffer.end(), request.begin(), request.end());

        if (!(result.second & IORING_CQE_F_SOCK_NONEMPTY)) {
            this->timer.update(connection.client.getFileDescriptorIndex(), connection.client.getTimeout());

            connection.generator = Generator{this->send(connection)};
            connection.generator.resume();
        }
    } else {
        this->timer.remove(connection.client.getFileDescriptorIndex());

        Log::produce(Log::formatLog(Log::Level::Warn, chrono::system_clock::now(), this_thread::get_id(),
                                    sourceLocation, strerror(abs(result.first))));

        connection.generator = Generator{this->cancel(connection)};
        connection.generator.resume();
    }
}

auto EventLoop::send(Connection &connection, source_location sourceLocation) -> Generator {
    const pair<int, unsigned int> result{co_await connection.client.send(
            this->userRing->getSqe(),
            Http::parse(string_view{reinterpret_cast<const char *>(connection.buffer.data()), connection.buffer.size()},
                        this->database))};

    connection.buffer.clear();

    if (result.first == 0 && result.second & IORING_CQE_F_NOTIF) {
        connection.generator = Generator{this->receive(connection)};
        connection.generator.resume();
    } else {
        this->timer.remove(connection.client.getFileDescriptorIndex());

        Log::produce(Log::formatLog(Log::Level::Warn, chrono::system_clock::now(), this_thread::get_id(),
                                    sourceLocation, strerror(abs(result.first))));

        connection.generator = Generator{this->cancel(connection)};
        connection.generator.resume();
    }
}

auto EventLoop::cancel(Connection &connection, source_location sourceLocation) -> Generator {
    const pair<int, unsigned int> result{co_await connection.client.cancel(this->userRing->getSqe())};

    if (result.first < 0)
        Log::produce(Log::formatLog(Log::Level::Error, chrono::system_clock::now(), this_thread::get_id(),
                                    sourceLocation, strerror(abs(result.first))));

    connection.generator = Generator{this->close(connection)};
    connection.generator.resume();
}

auto EventLoop::close(Connection &connection, source_location sourceLocation) -> Generator {
    const pair<int, unsigned int> result{co_await connection.client.close(this->userRing->getSqe())};

    if (result.first < 0)
        Log::produce(Log::formatLog(Log::Level::Error, chrono::system_clock::now(), this_thread::get_id(),
                                    sourceLocation, strerror(abs(result.first))));
}

EventLoop::~EventLoop() {
    EventLoop::instance = false;

    const int fileDescriptor{this->userRing->getSelfFileDescriptor()};

    const lock_guard lockGuard{EventLoop::lock};

    const auto result{
            ranges::find_if(EventLoop::cpus, [fileDescriptor](int value) { return value == fileDescriptor; })};

    if (result != EventLoop::cpus.end()) *result = -1;
    else
        Log::produce(Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                    source_location::current(), "can not find self file descriptor"));
}

constinit thread_local bool EventLoop::instance{false};
constinit mutex EventLoop::lock;
vector<int> EventLoop::cpus{vector<int>(jthread::hardware_concurrency(), -1)};
