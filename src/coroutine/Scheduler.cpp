module;

#include "../fileDescriptor/Client.hpp"
#include "../fileDescriptor/Logger.hpp"
#include "../fileDescriptor/Server.hpp"
#include "../fileDescriptor/Timer.hpp"
#include "../http/HttpParse.hpp"
#include "../log/Exception.hpp"
#include "../ring/Completion.hpp"
#include "../ring/Ring.hpp"
#include "../ring/RingBuffer.hpp"

#include <cstring>
#include <ranges>

export module Scheduler;

export class Scheduler {
public:
    static constexpr auto registerSignal(std::source_location sourceLocation = std::source_location::current())
        -> void {
        struct sigaction signalAction {};

        signalAction.sa_handler = [](int) noexcept { Scheduler::switcher.clear(std::memory_order_relaxed); };

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

    Scheduler() {
        this->ring->registerSelfFileDescriptor();

        {
            const std::lock_guard lockGuard{Scheduler::lock};

            const auto result{std::ranges::find(Scheduler::ringFileDescriptors, this->ring->getFileDescriptor())};
            this->ring->registerCpu(std::distance(Scheduler::ringFileDescriptors.begin(), result));
        }

        this->ring->registerSparseFileDescriptor(Ring::getFileDescriptorLimit());
        this->ring->allocateFileDescriptorRange(2, Ring::getFileDescriptorLimit() - 2);

        const std::array<int, 3> fileDescriptors{Logger::create(), Server::create(), Timer::create()};
        this->ring->updateFileDescriptors(0, fileDescriptors);
    }

    Scheduler(const Scheduler &) = delete;

    auto operator=(const Scheduler &) -> Scheduler & = delete;

    Scheduler(Scheduler &&) = delete;

    auto operator=(Scheduler &&) -> Scheduler & = delete;

    constexpr ~Scheduler() {
        this->closeAll();

        const std::lock_guard lockGuard{Scheduler::lock};

        auto result{std::ranges::find(Scheduler::ringFileDescriptors, this->ring->getFileDescriptor())};
        *result = -1;

        if (Scheduler::sharedRingFileDescriptor == this->ring->getFileDescriptor()) {
            Scheduler::sharedRingFileDescriptor = -1;

            result = std::ranges::find_if(Scheduler::ringFileDescriptors,
                                          [](const int fileDescriptor) noexcept { return fileDescriptor != -1; });
            if (result != Scheduler::ringFileDescriptors.cend()) Scheduler::sharedRingFileDescriptor = *result;
        }

        Scheduler::instance = false;
    }

    auto run() -> void {
        this->submit(std::make_shared<Task>(this->accept()));
        this->submit(std::make_shared<Task>(this->timing()));

        while (Scheduler::switcher.test(std::memory_order::relaxed)) {
            if (this->logger->writable()) this->submit(std::make_shared<Task>(this->write()));

            this->ring->wait(1);
            this->frame();
        }
    }

private:
    [[nodiscard]] static constexpr auto
        initializeRing(std::source_location sourceLocation = std::source_location::current()) -> std::shared_ptr<Ring> {
        if (Scheduler::instance) {
            throw Exception{
                Log{Log::Level::fatal, "one thread can only have one Scheduler", sourceLocation}
            };
        }
        Scheduler::instance = true;

        io_uring_params params{};
        params.flags = IORING_SETUP_CLAMP | IORING_SETUP_SUBMIT_ALL | IORING_SETUP_COOP_TASKRUN |
                       IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

        const std::lock_guard lockGuard{Scheduler::lock};

        if (Scheduler::sharedRingFileDescriptor != -1) {
            params.wq_fd = Scheduler::sharedRingFileDescriptor;
            params.flags |= IORING_SETUP_ATTACH_WQ;
        }

        auto ring{std::make_shared<Ring>(2048 / Scheduler::ringFileDescriptors.size(), params)};

        if (Scheduler::sharedRingFileDescriptor == -1) Scheduler::sharedRingFileDescriptor = ring->getFileDescriptor();

        const auto result{std::ranges::find(Scheduler::ringFileDescriptors, -1)};
        if (result != Scheduler::ringFileDescriptors.cend()) *result = ring->getFileDescriptor();
        else {
            throw Exception{
                Log{Log::Level::fatal, "too many Scheduler", sourceLocation}
            };
        }

        return ring;
    }

    constexpr auto frame() -> void {
        const int completionCount{this->ring->poll([this](const Completion &completion) {
            if (completion.outcome.result != 0 || !(completion.outcome.flags & IORING_CQE_F_NOTIF)) {
                this->currentUserData = completion.userData;
                const std::shared_ptr<Task> task{this->tasks.at(this->currentUserData)};
                task->resume(completion.outcome);
            }
        })};
        this->ring->advance(this->ringBuffer.getHandle(), completionCount, this->ringBuffer.getAddedBufferCount());
    }

    constexpr auto submit(std::shared_ptr<Task> &&task) -> void {
        task->resume(Outcome{});
        this->ring->submit(task->getSubmission());
        this->tasks.emplace(task->getSubmission().userData, std::move(task));
    }

    constexpr auto eraseCurrentTask() -> void { this->tasks.erase(this->currentUserData); }

    [[nodiscard]] auto write(std::source_location sourceLocation = std::source_location::current()) -> Task {
        const Outcome outcome{co_await this->logger->write()};
        if (outcome.result < 0) {
            throw Exception{
                Log{Log::Level::error, std::strerror(std::abs(outcome.result)), sourceLocation}
            };
        }
        this->logger->wrote();

        this->eraseCurrentTask();
    }

    [[nodiscard]] auto accept(std::source_location sourceLocation = std::source_location::current()) -> Task {
        while (true) {
            const Outcome outcome{co_await this->server.accept()};
            if (outcome.result >= 0 && outcome.flags & IORING_CQE_F_MORE) {
                this->clients.emplace(outcome.result, Client{outcome.result, 60});

                const Client &client{this->clients.at(outcome.result)};

                this->timer.add(outcome.result, client.getSeconds());
                this->submit(std::make_shared<Task>(this->receive(client)));
            } else {
                this->eraseCurrentTask();

                throw Exception{
                    Log{Log::Level::error, std::strerror(std::abs(outcome.result)), sourceLocation}
                };
            }
        }
    }

    [[nodiscard]] auto timing(std::source_location sourceLocation = std::source_location::current()) -> Task {
        const Outcome outcome{co_await this->timer.timing()};
        if (outcome.result == sizeof(unsigned long)) {
            for (const int fileDescriptor : this->timer.clearTimeout())
                this->submit(std::make_shared<Task>(this->cancel(fileDescriptor)));

            this->submit(std::make_shared<Task>(this->timing()));
        } else {
            throw Exception{
                Log{Log::Level::error, std::strerror(std::abs(outcome.result)), sourceLocation}
            };
        }

        this->eraseCurrentTask();
    }

    [[nodiscard]] auto receive(const Client &client,
                               std::source_location sourceLocation = std::source_location::current()) -> Task {
        std::vector<std::byte> buffer;

        while (true) {
            const Outcome outcome{co_await client.receive(this->ringBuffer.getId())};
            if (outcome.result > 0 && outcome.flags & IORING_CQE_F_MORE) {
                const std::span<const std::byte> receivedData{
                    this->ringBuffer.readFromBuffer(outcome.flags >> IORING_CQE_BUFFER_SHIFT, outcome.result)};
                buffer.insert(buffer.cend(), receivedData.cbegin(), receivedData.cend());

                if (!(outcome.flags & IORING_CQE_F_SOCK_NONEMPTY)) {
                    this->timer.update(client.getFileDescriptor(), client.getSeconds());

                    std::vector<std::byte> response{this->httpParse.parse(
                        std::string_view{reinterpret_cast<const char *>(buffer.data()), buffer.size()})};
                    buffer.clear();

                    this->submit(std::make_shared<Task>(this->send(client, std::move(response))));
                }
            } else {
                this->logger->push(Log{Log::Level::warn, std::strerror(std::abs(outcome.result)), sourceLocation});

                this->timer.remove(client.getFileDescriptor());
                this->submit(std::make_shared<Task>(this->close(client.getFileDescriptor())));

                break;
            }
        }

        this->eraseCurrentTask();
    }

    [[nodiscard]] auto send(const Client &client, std::vector<std::byte> &&data,
                            std::source_location sourceLocation = std::source_location::current()) -> Task {
        const std::vector<std::byte> response{std::move(data)};
        const Outcome outcome{co_await client.send(response)};
        if (outcome.result <= 0) {
            this->logger->push(Log{Log::Level::warn, std::strerror(std::abs(outcome.result)), sourceLocation});

            this->timer.remove(client.getFileDescriptor());
            this->submit(std::make_shared<Task>(this->close(client.getFileDescriptor())));
        }

        this->eraseCurrentTask();
    }

    [[nodiscard]] auto cancel(int fileDescriptor, std::source_location sourceLocation = std::source_location::current())
        -> Task {
        Outcome outcome;
        if (fileDescriptor == this->logger->getFileDescriptor()) outcome = co_await this->logger->cancel();
        else if (fileDescriptor == this->server.getFileDescriptor()) outcome = co_await this->server.cancel();
        else if (fileDescriptor == this->timer.getFileDescriptor()) outcome = co_await this->timer.cancel();
        else [[likely]] outcome = co_await this->clients.at(fileDescriptor).cancel();

        if (outcome.result < 0)
            this->logger->push(Log{Log::Level::warn, std::strerror(std::abs(outcome.result)), sourceLocation});

        this->eraseCurrentTask();
    }

    [[nodiscard]] auto close(int fileDescriptor, std::source_location sourceLocation = std::source_location::current())
        -> Task {
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

    auto closeAll() -> void {
        for (const Client &client : this->clients | std::views::values)
            this->submit(std::make_shared<Task>(this->close(client.getFileDescriptor())));
        this->submit(std::make_shared<Task>(this->close(this->timer.getFileDescriptor())));
        this->submit(std::make_shared<Task>(this->close(this->server.getFileDescriptor())));
        this->submit(std::make_shared<Task>(this->close(this->logger->getFileDescriptor())));

        this->ring->wait(3 + this->clients.size());
        this->frame();
    }

    static constinit thread_local bool instance;
    static constinit std::mutex lock;
    static constinit int sharedRingFileDescriptor;
    static std::vector<int> ringFileDescriptors;
    static constinit std::atomic_flag switcher;

    const std::shared_ptr<Ring> ring{Scheduler::initializeRing()};
    const std::shared_ptr<Logger> logger{std::make_shared<Logger>(0)};
    const Server server{1};
    Timer timer{2};
    HttpParse httpParse{this->logger};
    std::unordered_map<int, Client> clients;
    RingBuffer ringBuffer{
        this->ring, static_cast<unsigned int>(std::bit_ceil(2048 / Scheduler::ringFileDescriptors.size())), 1024, 0};
    std::unordered_map<unsigned long, std::shared_ptr<Task>> tasks;
    unsigned long currentUserData{};
};

constinit thread_local bool Scheduler::instance{};
constinit std::mutex Scheduler::lock;
constinit int Scheduler::sharedRingFileDescriptor{-1};
std::vector<int> Scheduler::ringFileDescriptors{std::vector<int>(std::thread::hardware_concurrency(), -1)};
constinit std::atomic_flag Scheduler::switcher{true};
