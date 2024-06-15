#include "Scheduler.hpp"

#include "../fileDescriptor/Client.hpp"
#include "../log/Exception.hpp"
#include "../ring/Completion.hpp"
#include "../ring/Ring.hpp"

#include <cstring>
#include <ranges>

auto Scheduler::registerSignal(const std::source_location sourceLocation) -> void {
    struct sigaction signalAction {};

    signalAction.sa_handler = [](int) noexcept { switcher.clear(std::memory_order_relaxed); };

    if (sigaction(SIGTERM, &signalAction, nullptr) == -1) {
        throw Exception{
            Log{Log::Level::fatal, std::strerror(errno), sourceLocation}
        };
    }

    if (sigaction(SIGINT, &signalAction, nullptr) == -1) {
        throw Exception{
            Log{Log::Level::fatal, std::strerror(errno), sourceLocation}
        };
    }
}

Scheduler::Scheduler(const int sharedFileDescriptor, const unsigned int cpuCode) :
    ring{[sharedFileDescriptor] {
        io_uring_params params{};
        params.flags = IORING_SETUP_CLAMP | IORING_SETUP_SUBMIT_ALL | IORING_SETUP_COOP_TASKRUN |
                       IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

        if (sharedFileDescriptor != -1) {
            params.wq_fd = sharedFileDescriptor;
            params.flags |= IORING_SETUP_ATTACH_WQ;
        }

        auto ring{std::make_shared<Ring>(2048 / std::thread::hardware_concurrency(), params)};

        return ring;
    }()} {
    this->ring->registerSelfFileDescriptor();
    this->ring->registerCpu(cpuCode);
    this->ring->registerSparseFileDescriptor(Ring::getFileDescriptorLimit());

    const std::array fileDescriptors{Logger::create("log.log"), Server::create("127.0.0.1", 8080), Timer::create()};
    this->ring->allocateFileDescriptorRange(fileDescriptors.size(),
                                            Ring::getFileDescriptorLimit() - fileDescriptors.size());
    this->ring->updateFileDescriptors(0, fileDescriptors);
}

Scheduler::~Scheduler() {
    for (const auto &client : this->clients | std::views::values)
        this->submit(std::make_shared<Task>(this->close(client.getFileDescriptor())));
    this->submit(std::make_shared<Task>(this->close(this->timer.getFileDescriptor())));
    this->submit(std::make_shared<Task>(this->close(this->server.getFileDescriptor())));
    this->submit(std::make_shared<Task>(this->close(this->logger->getFileDescriptor())));

    this->ring->wait(3 + this->clients.size());
    this->frame();
}

auto Scheduler::getRingFileDescriptor() const noexcept -> int { return this->ring->getFileDescriptor(); }

auto Scheduler::run() -> void {
    this->submit(std::make_shared<Task>(this->accept()));
    this->submit(std::make_shared<Task>(this->timing()));

    while (switcher.test(std::memory_order::relaxed)) {
        if (this->logger->writable()) this->submit(std::make_shared<Task>(this->write()));

        this->ring->wait(1);
        this->frame();
    }
}

auto Scheduler::frame() -> void {
    const int completionCount{this->ring->poll([this](const Completion &completion) {
        if (completion.outcome.result != 0 || !(completion.outcome.flags & IORING_CQE_F_NOTIF)) {
            this->currentUserData = completion.userData;
            const std::shared_ptr task{this->tasks.at(this->currentUserData)};
            task->resume(completion.outcome);
        }
    })};
    this->ring->advance(this->ringBuffer.getHandle(), completionCount, this->ringBuffer.getAddedBufferCount());
}

auto Scheduler::submit(std::shared_ptr<Task> &&task) -> void {
    task->resume(Outcome{});
    this->ring->submit(task->getSubmission());
    this->tasks.emplace(task->getSubmission().userData, std::move(task));
}

auto Scheduler::eraseCurrentTask() -> void { this->tasks.erase(this->currentUserData); }

auto Scheduler::write(const std::source_location sourceLocation) -> Task {
    if (const auto [result, flags]{co_await this->logger->write()}; result < 0) {
        throw Exception{
            Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}
        };
    }
    this->logger->wrote();

    this->eraseCurrentTask();
}

auto Scheduler::accept(const std::source_location sourceLocation) -> Task {
    while (true) {
        if (const auto [result, flags]{co_await this->server.accept()}; result >= 0 && flags & IORING_CQE_F_MORE) {
            this->clients.emplace(result, Client{result, std::chrono::seconds{60}});

            const Client &client{this->clients.at(result)};

            this->timer.add(result, client.getSeconds());
            this->submit(std::make_shared<Task>(this->receive(client)));
        } else {
            this->eraseCurrentTask();

            throw Exception{
                Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}
            };
        }
    }
}

auto Scheduler::timing(const std::source_location sourceLocation) -> Task {
    if (const auto [result, flags]{co_await this->timer.timing()}; result == sizeof(unsigned long)) {
        for (const auto fileDescriptor : this->timer.clearTimeout())
            this->submit(std::make_shared<Task>(this->cancel(this->clients.at(fileDescriptor))));

        this->submit(std::make_shared<Task>(this->timing()));
    } else {
        throw Exception{
            Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}
        };
    }

    this->eraseCurrentTask();
}

auto Scheduler::receive(const Client &client, const std::source_location sourceLocation) -> Task {
    std::vector<std::byte> buffer;

    while (true) {
        if (const auto [result, flags]{co_await client.receive(this->ringBuffer.getId())};
            result > 0 && flags & IORING_CQE_F_MORE) {
            const std::span receivedData{this->ringBuffer.readFromBuffer(flags >> IORING_CQE_BUFFER_SHIFT, result)};
            buffer.insert(buffer.cend(), receivedData.cbegin(), receivedData.cend());

            if (!(flags & IORING_CQE_F_SOCK_NONEMPTY)) {
                this->timer.update(client.getFileDescriptor(), client.getSeconds());

                std::vector response{this->httpParse.parse(
                    std::string_view{reinterpret_cast<const char *>(buffer.data()), buffer.size()})};
                buffer.clear();

                this->submit(std::make_shared<Task>(this->send(client, std::move(response))));
            }
        } else {
            this->logger->push(Log{
                Log::Level::warn, result == 0 ? "connection closed" : std::strerror(std::abs(result)), sourceLocation});

            this->timer.remove(client.getFileDescriptor());
            this->submit(std::make_shared<Task>(this->close(client.getFileDescriptor())));

            break;
        }
    }

    this->eraseCurrentTask();
}

auto Scheduler::send(const Client &client, std::vector<std::byte> &&data, const std::source_location sourceLocation)
    -> Task {
    const std::vector response{std::move(data)};
    if (const auto [result, flags]{co_await client.send(response)}; result <= 0) {
        this->logger->push(
            Log{Log::Level::warn, result == 0 ? "connection closed" : std::strerror(std::abs(result)), sourceLocation});

        this->timer.remove(client.getFileDescriptor());
        this->submit(std::make_shared<Task>(this->close(client.getFileDescriptor())));
    }

    this->eraseCurrentTask();
}

auto Scheduler::cancel(const Client &client, const std::source_location sourceLocation) -> Task {
    if (const auto [result, flags]{co_await client.cancel()}; result < 0)
        this->logger->push(Log{Log::Level::warn, std::strerror(std::abs(result)), sourceLocation});

    this->eraseCurrentTask();
}

auto Scheduler::close(const int fileDescriptor, const std::source_location sourceLocation) -> Task {
    Outcome outcome;
    if (fileDescriptor == this->logger->getFileDescriptor()) outcome = co_await this->logger->close();
    else if (fileDescriptor == this->server.getFileDescriptor()) outcome = co_await this->server.close();
    else if (fileDescriptor == this->timer.getFileDescriptor()) outcome = co_await this->timer.close();
    else [[likely]] {
        outcome = co_await this->clients.at(fileDescriptor).close();
        this->clients.erase(fileDescriptor);
    }

    if (outcome.result < 0)
        this->logger->push(Log{Log::Level::warn, std::strerror(std::abs(outcome.result)), sourceLocation});

    this->eraseCurrentTask();
}

constinit std::atomic_flag Scheduler::switcher{true};
