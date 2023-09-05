#include "EventLoop.h"

#include "../base/Completion.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../http/Http.h"
#include "../log/Log.h"

#include <algorithm>
#include <cstring>

using namespace std;

EventLoop::EventLoop() noexcept
    : userRing{[](source_location sourceLocation = source_location::current()) noexcept {
          if (EventLoop::instance) terminate();
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
                  terminate();

              tempUserRing->registerCpu(std::distance(EventLoop::cpus.begin(), result));
          }

          return tempUserRing;
      }()},
      bufferRing{EventLoop::bufferRingEntries, EventLoop::bufferRingBufferSize, EventLoop::bufferRingId,
                 this->userRing},
      server{0}, timer{1}, database{{}, "AomaYple", "38820233", "webServer", 0, {}, 0} {
    this->userRing->registerSelfFileDescriptor();

    this->userRing->registerSparseFileDescriptors(UserRing::getFileDescriptorLimit());

    this->userRing->allocateFileDescriptorRange(2, UserRing::getFileDescriptorLimit() - 2);

    const array<unsigned int, 2> fileDescriptors{Server::create(EventLoop::port), Timer::create()};

    this->userRing->updateFileDescriptors(0, fileDescriptors);
}

EventLoop::EventLoop(EventLoop &&other) noexcept
    : userRing{std::move(other.userRing)}, bufferRing{std::move(other.bufferRing)}, server{std::move(other.server)},
      timer{std::move(other.timer)}, database{std::move(other.database)}, clients{std::move(other.clients)},
      generators{std::move(other.generators)} {}

auto EventLoop::loop() noexcept -> void {
    Generator generator{this->accept()};
    generator.resume();

    this->generators[static_cast<unsigned char>(EventType::Accept)].emplace(this->server.getFileDescriptorIndex(),
                                                                            std::move(generator));

    generator = this->timing();
    generator.resume();

    this->generators[static_cast<unsigned char>(EventType::Timeout)].emplace(this->timer.getFileDescriptorIndex(),
                                                                             std::move(generator));

    while (true) {
        this->userRing->submitWait(1);

        unsigned int completionCount{this->userRing->forEachCompletion([this](io_uring_cqe *cqe) {
            const Completion completion{cqe};

            const unsigned long completionUserData{completion.getUserData()};
            const UserData userData{reinterpret_cast<const UserData &>(completionUserData)};

            switch (userData.eventType) {
                case EventType::Accept:
                    this->server.setResult({completion.getResult(), completion.getFlags()});

                    this->generators[static_cast<unsigned char>(EventType::Accept)]
                            .at(userData.fileDescriptor)
                            .resume();

                    break;
                case EventType::Timeout:
                    this->timer.setResult({completion.getResult(), completion.getFlags()});

                    this->generators[static_cast<unsigned char>(EventType::Timeout)]
                            .at(userData.fileDescriptor)
                            .resume();

                    break;
                case EventType::Receive:
                    this->clients.at(userData.fileDescriptor)
                            .setResult({completion.getResult(), completion.getFlags()});

                    try {
                        this->generators[static_cast<unsigned char>(EventType::Receive)]
                                .at(userData.fileDescriptor)
                                .resume();
                    } catch (const Exception &exception) { Log::produce(exception.what()); }

                    break;
                case EventType::Send:
                    if (completion.getResult() <= 0) {
                        this->clients.at(userData.fileDescriptor)
                                .setResult({completion.getResult(), completion.getFlags()});

                        this->generators[static_cast<unsigned char>(EventType::Send)].at(userData.fileDescriptor);
                    }

                    break;
                case EventType::Cancel:
                    this->clients.at(userData.fileDescriptor)
                            .setResult({completion.getResult(), completion.getFlags()});

                    this->generators[static_cast<unsigned char>(EventType::Cancel)].at(userData.fileDescriptor);

                    break;
                case EventType::Close:
                    this->clients.at(userData.fileDescriptor)
                            .setResult({completion.getResult(), completion.getFlags()});

                    this->generators[static_cast<unsigned char>(EventType::Close)].at(userData.fileDescriptor);

                    break;
            }
        })};

        this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
    }
}

auto EventLoop::accept() noexcept -> Generator {
    this->server.startAccept(this->userRing->getSqe());

    while (true) {
        const pair<int, unsigned int> result{co_await this->server.accept()};

        if (!(result.second & IORING_CQE_F_MORE)) terminate();

        if (result.first >= 0) {
            Client client{static_cast<unsigned int>(result.first), 60};

            this->timer.add(result.first, client.getTimeout());

            this->clients.emplace(result.first, std::move(client));

            Generator generator{this->receive(result.first)};
            generator.resume();

            this->generators[static_cast<unsigned char>(EventType::Receive)].emplace(result.first,
                                                                                     std::move(generator));
        } else
            terminate();
    }
}

auto EventLoop::timing() noexcept -> Generator {
    const pair<int, unsigned int> result{co_await this->timer.timing(this->userRing->getSqe())};

    if (result.first == sizeof(unsigned long))
        ranges::for_each(this->timer.clearTimeout(), [this](unsigned int fileDescriptor) {
            this->timer.remove(fileDescriptor);

            Generator generator{this->cancel(fileDescriptor)};
            generator.resume();

            this->generators[static_cast<unsigned char>(EventType::Cancel)].emplace(fileDescriptor,
                                                                                    std::move(generator));
        });
    else
        terminate();
}

auto EventLoop::receive(unsigned int fileDescriptor, source_location sourceLocation) -> Generator {
    Client &client{this->clients.at(fileDescriptor)};

    client.startReceive(this->userRing->getSqe(), this->bufferRing.getId());

    while (true) {
        const pair<int, unsigned int> result{co_await client.receive()};

        if (result.first > 0) {
            if (!(result.second & IORING_CQE_F_MORE)) terminate();

            client.writeReceivedData(this->bufferRing.getData(result.second >> IORING_CQE_BUFFER_SHIFT, result.first));

            if (!(result.second & IORING_CQE_F_SOCK_NONEMPTY)) {
                this->timer.update(fileDescriptor, client.getTimeout());

                Generator generator{this->send(fileDescriptor)};
                generator.resume();


                auto &sendGenerators{this->generators[static_cast<unsigned char>(EventType::Send)]};
                if (sendGenerators.contains(fileDescriptor)) sendGenerators.at(fileDescriptor) = std::move(generator);
                else
                    sendGenerators.emplace(fileDescriptor, std::move(generator));
            }
        } else {
            this->timer.remove(fileDescriptor);

            Generator generator{this->cancel(fileDescriptor)};
            generator.resume();

            this->generators[static_cast<unsigned char>(EventType::Cancel)].emplace(fileDescriptor,
                                                                                    std::move(generator));

            throw Exception{Log::formatLog(Log::Level::Info, chrono::system_clock::now(), this_thread::get_id(),
                                           sourceLocation, "receive error: " + string{strerror(abs(result.first))})};
        }
    }
}

auto EventLoop::send(unsigned int fileDescriptor) noexcept -> Generator {
    Client &client{this->clients.at(fileDescriptor)};

    const pair<int, unsigned int> result{
            co_await client.send(this->userRing->getSqe(), Http::parse(client.readReceivedData(), this->database))};

    if (result.first != 0 || !(result.second & IORING_CQE_F_NOTIF)) terminate();
}

auto EventLoop::cancel(unsigned int fileDescriptor) noexcept -> Generator {
    const pair<int, unsigned int> result{co_await this->clients.at(fileDescriptor).cancel(this->userRing->getSqe())};

    if (result.first != 0) terminate();

    Generator generator{this->close(fileDescriptor)};
    generator.resume();

    this->generators[static_cast<unsigned char>(EventType::Close)].emplace(fileDescriptor, std::move(generator));
}

auto EventLoop::close(unsigned int fileDescriptor) noexcept -> Generator {
    const pair<int, unsigned int> result{co_await this->clients.at(fileDescriptor).close(this->userRing->getSqe())};

    if (result.first != 0) terminate();

    this->clients.erase(fileDescriptor);
    ranges::for_each(this->generators, [fileDescriptor](unordered_map<unsigned int, Generator> &element) {
        element.erase(fileDescriptor);
    });
}

EventLoop::~EventLoop() {
    EventLoop::instance = false;

    const unsigned int fileDescriptor{this->userRing->getSelfFileDescriptor()};

    const lock_guard lockGuard{EventLoop::lock};

    const auto result{
            ranges::find_if(EventLoop::cpus, [fileDescriptor](int value) { return value == fileDescriptor; })};

    if (result != EventLoop::cpus.end()) *result = -1;
    else
        terminate();
}

constinit thread_local bool EventLoop::instance{false};
constinit mutex EventLoop::lock;
vector<int> EventLoop::cpus{vector<int>(jthread::hardware_concurrency(), -1)};
