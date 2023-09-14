#include "Scheduler.hpp"

#include "../http/Http.hpp"
#include "../log/Log.hpp"
#include "../userRing/Completion.hpp"
#include "../userRing/UserData.hpp"
#include "ScheduleError.hpp"

#include <algorithm>
#include <cstring>

Scheduler::Scheduler()
    : userRing{[](std::source_location sourceLocation = std::source_location::current()) {
          if (Scheduler::instance)
              throw ScheduleError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                                 std::this_thread::get_id(), sourceLocation,
                                                 "one scheduler instance per thread")};
          Scheduler::instance = true;

          io_uring_params params{};

          params.flags = IORING_SETUP_SUBMIT_ALL | IORING_SETUP_CLAMP | IORING_SETUP_COOP_TASKRUN |
                         IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

          std::shared_ptr<UserRing> tempUserRing;

          {
              const std::lock_guard lockGuard{Scheduler::lock};

              if (Scheduler::sharedFileDescriptor != -1) {
                  params.wq_fd = Scheduler::sharedFileDescriptor;
                  params.flags |= IORING_SETUP_ATTACH_WQ;
              }

              tempUserRing = std::make_shared<UserRing>(1024, params);

              if (Scheduler::sharedFileDescriptor == -1)
                  Scheduler::sharedFileDescriptor = tempUserRing->getSelfFileDescriptor();
          }

          return tempUserRing;
      }()},
      bufferRing{1024, 1024, 0, this->userRing}, server{0}, timer{1},
      database{{}, "AomaYple", "38820233", "webServer", 0, {}, 0} {
    this->userRing->registerSelfFileDescriptor();
    this->userRing->registerCpu(Scheduler::cpuCode++);
    this->userRing->registerSparseFileDescriptors(UserRing::getFileDescriptorLimit());
    this->userRing->allocateFileDescriptorRange(2, UserRing::getFileDescriptorLimit() - 2);

    const std::array<int, 2> fileDescriptors{static_cast<int>(Server::create(9999)), static_cast<int>(Timer::create())};

    this->userRing->updateFileDescriptors(0, fileDescriptors);
}

Scheduler::Scheduler(Scheduler &&other) noexcept
    : userRing{std::move(other.userRing)}, bufferRing{std::move(other.bufferRing)}, server{std::move(other.server)},
      timer{std::move(other.timer)}, database{std::move(other.database)}, clients{std::move(other.clients)} {}

auto Scheduler::run() -> void {
    Task task{this->accept()};
    task.resume();
    this->server.setAcceptTask(std::move(task));

    task = this->timing();
    task.resume();
    this->timer.setTimingTask(std::move(task));

    while (true) {
        this->userRing->submitWait(1);

        unsigned int completionCount{
                this->userRing->forEachCompletion([this](io_uring_cqe *cqe) { this->frame(cqe); })};

        this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
    }
}

auto Scheduler::frame(io_uring_cqe *cqe) -> void {
    const Completion completion{cqe};

    const unsigned long completionUserData{completion.getUserData()};
    const UserData userData{reinterpret_cast<const UserData &>(completionUserData)};

    const std::pair<int, unsigned int> result{completion.getResult(), completion.getFlags()};

    switch (userData.taskType) {
        case TaskType::Accept:
            this->server.resumeAccept(result);

            break;
        case TaskType::Timeout:
            this->timer.resumeTiming(result);

            break;
        case TaskType::Receive:
            if (result.first >= 0 || std::abs(result.first) != ECANCELED) {
                Client &client{this->clients.at(userData.fileDescriptor)};
                try {
                    client.resumeReceive(result);
                } catch (const ScheduleError &scheduleError) {
                    client.setReceiveTask(Task{nullptr});

                    Log::produce(scheduleError.what());
                }
            }
            break;
        case TaskType::Send: {
            if (result.first != 0 || !(result.second & IORING_CQE_F_NOTIF)) {
                Client &client{this->clients.at(userData.fileDescriptor)};

                try {
                    client.resumeSend(result);
                } catch (const ScheduleError &scheduleError) {
                    client.setSendTask(Task{nullptr});

                    Log::produce(scheduleError.what());
                }
            }
        } break;
        case TaskType::Cancel: {
            Client &client{this->clients.at(userData.fileDescriptor)};

            try {
                client.resumeCancel(result);
            } catch (const ScheduleError &scheduleError) { Log::produce(scheduleError.what()); }

            client.setCancelTask(Task{nullptr});
        }

        break;
        case TaskType::Close:
            try {
                this->clients.at(userData.fileDescriptor).resumeClose(result);
            } catch (const ScheduleError &scheduleError) { Log::produce(scheduleError.what()); }

            this->clients.erase(userData.fileDescriptor);

            break;
    }
}

auto Scheduler::accept(std::source_location sourceLocation) -> Task {
    this->server.startAccept(this->userRing->getSqe());

    while (true) {
        const std::pair<int, unsigned int> result{co_await this->server.accept()};

        if (result.first >= 0) {
            this->clients.emplace(result.first, Client{static_cast<unsigned int>(result.first), 60});

            Client &client{this->clients.at(result.first)};

            this->timer.add(result.first, client.getTimeout());

            Task task{this->receive(client)};
            task.resume();
            client.setReceiveTask(std::move(task));
        } else
            throw ScheduleError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result.first)))};
    }
}

auto Scheduler::timing(std::source_location sourceLocation) -> Task {
    while (true) {
        const std::pair<int, unsigned int> result{co_await this->timer.timing(this->userRing->getSqe())};

        if (result.first == sizeof(unsigned long)) {
            std::ranges::for_each(this->timer.clearTimeout(), [this](unsigned int fileDescriptor) {
                Client &client{this->clients.at(fileDescriptor)};

                Task task{this->cancel(client)};
                task.resume();
                client.setCancelTask(std::move(task));
            });
        } else
            throw ScheduleError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result.first)))};
    }
}

auto Scheduler::receive(Client &client, std::source_location sourceLocation) -> Task {
    client.startReceive(this->userRing->getSqe(), this->bufferRing.getId());

    while (true) {
        const std::pair<int, unsigned int> result{co_await client.receive()};

        if (result.first > 0) {
            client.writeData(this->bufferRing.getData(result.second >> IORING_CQE_BUFFER_SHIFT, result.first));

            if (!(result.second & IORING_CQE_F_SOCK_NONEMPTY)) {
                this->timer.update(client.getFileDescriptorIndex(), client.getTimeout());

                Task task{this->send(client)};
                task.resume();
                client.setSendTask(std::move(task));
            }
        } else {
            this->timer.remove(client.getFileDescriptorIndex());

            Task task{this->cancel(client)};
            task.resume();
            client.setCancelTask(std::move(task));

            throw ScheduleError{Log::formatLog(Log::Level::Warn, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result.first)))};
        }
    }
}

auto Scheduler::send(Client &client, std::source_location sourceLocation) -> Task {
    const std::span<const std::byte> request{client.readData()};
    std::vector<std::byte> response{Http::parse(
            std::string_view{reinterpret_cast<const char *>(request.data()), request.size()}, this->database)};

    client.clearBuffer();

    const std::pair<int, unsigned int> result{co_await client.send(this->userRing->getSqe(), std::move(response))};

    client.clearBuffer();

    if (result.first <= 0) {
        this->timer.remove(client.getFileDescriptorIndex());

        Task task{this->cancel(client)};
        task.resume();
        client.setCancelTask(std::move(task));

        throw ScheduleError{Log::formatLog(Log::Level::Warn, std::chrono::system_clock::now(),
                                           std::this_thread::get_id(), sourceLocation,
                                           std::strerror(std::abs(result.first)))};
    }
}

auto Scheduler::cancel(Client &client, std::source_location sourceLocation) const -> Task {
    const std::pair<int, unsigned int> result{co_await client.cancel(this->userRing->getSqe())};

    Task task{this->close(client)};
    task.resume();
    client.setCloseTask(std::move(task));

    if (result.first < 0)
        throw ScheduleError{Log::formatLog(Log::Level::Error, std::chrono::system_clock::now(),
                                           std::this_thread::get_id(), sourceLocation,
                                           std::strerror(std::abs(result.first)))};
}

auto Scheduler::close(const Client &client, std::source_location sourceLocation) const -> Task {
    const std::pair<int, unsigned int> result{co_await client.close(this->userRing->getSqe())};

    if (result.first < 0)
        throw ScheduleError{Log::formatLog(Log::Level::Error, std::chrono::system_clock::now(),
                                           std::this_thread::get_id(), sourceLocation,
                                           std::strerror(std::abs(result.first)))};
}

constinit thread_local bool Scheduler::instance{false};
constinit std::mutex Scheduler::lock;
constinit int Scheduler::sharedFileDescriptor{-1};
constinit std::atomic_ushort Scheduler::cpuCode{0};
