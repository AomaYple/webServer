#include "Scheduler.hpp"

#include "../http/Http.hpp"
#include "../log/Log.hpp"
#include "../socket/Client.hpp"
#include "../userRing/Completion.hpp"
#include "../userRing/UserData.hpp"
#include "ScheduleError.hpp"

#include <cstring>

Scheduler::Scheduler()
    : userRing{[]() {
          Scheduler::judgeOneThreadOneInstance();

          io_uring_params params{};

          params.flags = IORING_SETUP_SUBMIT_ALL | IORING_SETUP_CLAMP | IORING_SETUP_COOP_TASKRUN |
                         IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

          std::shared_ptr<UserRing> tempUserRing;

          {
              const std::lock_guard lockGuard{Scheduler::lock};

              if (Scheduler::sharedUserRingFileDescriptor != -1) {
                  params.wq_fd = Scheduler::sharedUserRingFileDescriptor;
                  params.flags |= IORING_SETUP_ATTACH_WQ;
              }

              tempUserRing = std::make_shared<UserRing>(1024, params);

              if (Scheduler::sharedUserRingFileDescriptor == -1)
                  Scheduler::sharedUserRingFileDescriptor = static_cast<int>(tempUserRing->getSelfFileDescriptor());

              const auto result{std::ranges::find(Scheduler::userRingFileDescriptors, -1)};
              *result = static_cast<int>(tempUserRing->getSelfFileDescriptor());

              tempUserRing->registerCpu(std::distance(Scheduler::userRingFileDescriptors.begin(), result));
          }

          return tempUserRing;
      }()},
      bufferRing{1024, 1024, 0, this->userRing}, server{0}, timer{1},
      database{{}, "AomaYple", "38820233", "webServer", 0, {}, 0} {
    this->userRing->registerSelfFileDescriptor();
    this->userRing->registerSparseFileDescriptors(UserRing::getFileDescriptorLimit());
    this->userRing->allocateFileDescriptorRange(2, UserRing::getFileDescriptorLimit() - 2);

    const std::array<unsigned int, 2> fileDescriptors{Server::create(9999), Timer::create()};

    this->userRing->updateFileDescriptors(0, fileDescriptors);
}

Scheduler::~Scheduler() {
    this->releaseResources();

    Scheduler::instance = false;

    const std::lock_guard lockGuard{Scheduler::lock};

    auto result{std::ranges::find(Scheduler::userRingFileDescriptors,
                                  static_cast<int>(this->userRing->getSelfFileDescriptor()))};
    *result = -1;

    if (Scheduler::sharedUserRingFileDescriptor == static_cast<int>(this->userRing->getSelfFileDescriptor())) {
        Scheduler::sharedUserRingFileDescriptor = -1;

        result = std::ranges::find_if(Scheduler::userRingFileDescriptors,
                                      [](const int fileDescriptor) { return fileDescriptor != -1; });

        if (result != Scheduler::userRingFileDescriptors.cend()) Scheduler::sharedUserRingFileDescriptor = *result;
    }
}

auto Scheduler::judgeOneThreadOneInstance(std::source_location sourceLocation) -> void {
    if (Scheduler::instance)
        throw ScheduleError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                           std::this_thread::get_id(), sourceLocation,
                                           "one scheduler instance per thread")};
    Scheduler::instance = true;
}

auto Scheduler::releaseResources() -> void {
    Generator generator{this->closeServer()};
    generator.resume();
    this->server.setCloseGenerator(std::move(generator));

    generator = this->closeTimer();
    generator.resume();
    this->timer.setCloseGenerator(std::move(generator));

    for (auto &client: this->clients) {
        generator = this->closeClient(client.second);
        generator.resume();
        client.second.setCloseGenerator(std::move(generator));
    }

    this->userRing->submitWait(2 + this->clients.size());

    const unsigned int completionCount{this->userRing->forEachCompletion([this](io_uring_cqe *cqe) {
        const Completion completion{cqe};

        const unsigned long completionUserData{completion.getUserData()};
        const UserData userData{std::bit_cast<UserData>(completionUserData)};

        const std::pair<int, unsigned int> result{completion.getResult(), completion.getFlags()};

        if (userData.fileDescriptor == this->server.getFileDescriptorIndex()) this->server.resumeClose(result);
        else if (userData.fileDescriptor == this->timer.getFileDescriptorIndex())
            this->timer.resumeClose(result);
        else {
            Client &client{this->clients.at(userData.fileDescriptor)};

            try {
                client.resumeClose(result);
            } catch (const ScheduleError &scheduleError) { Log::produce(scheduleError.what()); }

            this->clients.erase(userData.fileDescriptor);
        }
    })};

    this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
}

auto Scheduler::run() -> void {
    Generator generator{this->accept()};
    generator.resume();
    this->server.setAcceptGenerator(std::move(generator));

    generator = this->timing();
    generator.resume();
    this->timer.setTimingGenerator(std::move(generator));

    while (true) {
        this->userRing->submitWait(1);

        const unsigned int completionCount{
                this->userRing->forEachCompletion([this](io_uring_cqe *cqe) { this->frame(cqe); })};

        this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
    }
}

auto Scheduler::frame(io_uring_cqe *cqe) -> void {
    const Completion completion{cqe};

    const unsigned long completionUserData{completion.getUserData()};
    const UserData userData{std::bit_cast<UserData>(completionUserData)};

    const std::pair<int, unsigned int> result{completion.getResult(), completion.getFlags()};

    switch (userData.eventType) {
        case EventType::Accept:
            this->server.resumeAccept(result);

            break;
        case EventType::Timeout:
            this->timer.resumeTiming(result);

            break;
        case EventType::Receive:
            if (result.first >= 0 || std::abs(result.first) != ECANCELED) {
                Client &client{this->clients.at(userData.fileDescriptor)};

                try {
                    client.resumeReceive(result);
                } catch (const ScheduleError &scheduleError) {
                    client.setReceiveGenerator(Generator{});

                    Log::produce(scheduleError.what());
                }
            }

            break;
        case EventType::Send:
            if (!(result.second & IORING_CQE_F_NOTIF)) {
                Client &client{this->clients.at(userData.fileDescriptor)};

                try {
                    client.resumeSend(result);
                } catch (const ScheduleError &scheduleError) { Log::produce(scheduleError.what()); }

                client.setSendGenerator(Generator{});
            }

            break;
        case EventType::Cancel: {
            Client &client{this->clients.at(userData.fileDescriptor)};

            try {
                client.resumeCancel(result);
            } catch (const ScheduleError &scheduleError) { Log::produce(scheduleError.what()); }

            client.setCancelGenerator(Generator{});
        }

        break;
        case EventType::Close:
            try {
                this->clients.at(userData.fileDescriptor).resumeClose(result);
            } catch (const ScheduleError &scheduleError) { Log::produce(scheduleError.what()); }

            this->clients.erase(userData.fileDescriptor);

            break;
    }
}

auto Scheduler::accept(std::source_location sourceLocation) -> Generator {
    this->server.startAccept(this->userRing->getSqe());

    while (true) {
        const std::pair<int, unsigned int> result{co_await this->server.accept()};

        if (result.first >= 0) {
            this->clients.emplace(result.first, Client{static_cast<unsigned int>(result.first), 60});

            Client &client{this->clients.at(result.first)};

            this->timer.add(result.first, client.getTimeout());

            Generator generator{this->receive(client)};
            generator.resume();
            client.setReceiveGenerator(std::move(generator));
        } else
            throw ScheduleError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result.first)))};
    }
}

auto Scheduler::timing(std::source_location sourceLocation) -> Generator {
    while (true) {
        const std::pair<int, unsigned int> result{co_await this->timer.timing(this->userRing->getSqe())};

        if (result.first == sizeof(unsigned long)) {
            for (const unsigned int fileDescriptor: this->timer.clearTimeout()) {
                Client &client{this->clients.at(fileDescriptor)};

                Generator generator{this->cancelClient(client)};
                generator.resume();
                client.setCancelGenerator(std::move(generator));
            }
        } else
            throw ScheduleError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result.first)))};
    }
}

auto Scheduler::receive(Client &client, std::source_location sourceLocation) -> Generator {
    client.startReceive(this->userRing->getSqe(), this->bufferRing.getId());

    while (true) {
        const std::pair<int, unsigned int> result{co_await client.receive()};

        if (result.first > 0) {
            client.writeData(this->bufferRing.getData(result.second >> IORING_CQE_BUFFER_SHIFT, result.first));

            if (!(result.second & IORING_CQE_F_SOCK_NONEMPTY)) {
                this->timer.update(client.getFileDescriptorIndex(), client.getTimeout());

                Generator generator{this->send(client)};
                generator.resume();
                client.setSendGenerator(std::move(generator));
            }
        } else {
            this->timer.remove(client.getFileDescriptorIndex());

            Generator generator{this->cancelClient(client)};
            generator.resume();
            client.setCancelGenerator(std::move(generator));

            throw ScheduleError{Log::formatLog(Log::Level::Error, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result.first)))};
        }
    }
}

auto Scheduler::send(Client &client, std::source_location sourceLocation) -> Generator {
    const std::span<const std::byte> request{client.readData()};
    std::vector<std::byte> response{Http::parse(
            std::string_view{reinterpret_cast<const char *>(request.data()), request.size()}, this->database)};

    client.clearBuffer();

    const std::pair<int, unsigned int> result{co_await client.send(this->userRing->getSqe(), std::move(response))};

    client.clearBuffer();

    if (result.first <= 0) {
        this->timer.remove(client.getFileDescriptorIndex());

        Generator generator{this->cancelClient(client)};
        generator.resume();
        client.setCancelGenerator(std::move(generator));

        throw ScheduleError{Log::formatLog(Log::Level::Error, std::chrono::system_clock::now(),
                                           std::this_thread::get_id(), sourceLocation,
                                           std::strerror(std::abs(result.first)))};
    }
}

auto Scheduler::cancelClient(Client &client, std::source_location sourceLocation) -> Generator {
    const std::pair<int, unsigned int> result{co_await client.cancel(this->userRing->getSqe())};

    Generator generator{this->closeClient(client)};
    generator.resume();
    client.setCloseGenerator(std::move(generator));

    if (result.first < 0)
        throw ScheduleError{Log::formatLog(Log::Level::Error, std::chrono::system_clock::now(),
                                           std::this_thread::get_id(), sourceLocation,
                                           std::strerror(std::abs(result.first)))};
}

auto Scheduler::closeClient(const Client &client, std::source_location sourceLocation) -> Generator {
    const std::pair<int, unsigned int> result{co_await client.close(this->userRing->getSqe())};

    if (result.first < 0)
        throw ScheduleError{Log::formatLog(Log::Level::Error, std::chrono::system_clock::now(),
                                           std::this_thread::get_id(), sourceLocation,
                                           std::strerror(std::abs(result.first)))};
}

auto Scheduler::closeServer(std::source_location sourceLocation) -> Generator {
    const std::pair<int, unsigned int> result{co_await this->server.close(this->userRing->getSqe())};

    if (result.first < 0)
        throw ScheduleError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                           std::this_thread::get_id(), sourceLocation,
                                           std::strerror(std::abs(result.first)))};
}

auto Scheduler::closeTimer(std::source_location sourceLocation) -> Generator {
    const std::pair<int, unsigned int> result{co_await this->timer.close(this->userRing->getSqe())};

    if (result.first < 0)
        throw ScheduleError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                           std::this_thread::get_id(), sourceLocation,
                                           std::strerror(std::abs(result.first)))};
}

constinit thread_local bool Scheduler::instance{false};
constinit std::mutex Scheduler::lock;
constinit int Scheduler::sharedUserRingFileDescriptor{-1};
std::vector<int> Scheduler::userRingFileDescriptors{std::vector<int>(std::jthread::hardware_concurrency(), -1)};
