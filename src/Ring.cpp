#include "Ring.h"

#include <sys/resource.h>
#include <sys/sysinfo.h>

#include <cstring>

#include "Log.h"

using std::string, std::vector, std::runtime_error, std::mutex, std::lock_guard, std::ranges::find_if,
        std::source_location;

constexpr unsigned int ringEntries{2048};

mutex Ring::lock{};
vector<int> Ring::cpus{vector<int>(get_nprocs(), -1)};

thread_local bool Ring::instance{false};

Ring::Ring() : self{} {
    if (Ring::instance) throw runtime_error("ring one thread one instance");
    Ring::instance = true;

    io_uring_params params{};
    params.flags = IORING_SETUP_CLAMP | IORING_SETUP_SUBMIT_ALL | IORING_SETUP_COOP_TASKRUN |
                   IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

    {
        lock_guard lockGuard{Ring::lock};

        auto result{find_if(Ring::cpus, [](int i) { return i != -1; })};

        if (result != Ring::cpus.end()) {
            params.flags |= IORING_SETUP_ATTACH_WQ;
            params.wq_fd = *result;
        }
    }

    int returnValue{io_uring_queue_init_params(ringEntries, &this->self, &params)};
    if (returnValue != 0) throw runtime_error("ring initialize error: " + string{std::strerror(std::abs(returnValue))});

    {
        lock_guard lockGuard{Ring::lock};

        auto result{find_if(Ring::cpus, [](int i) { return i != -1; })};

        if (result == Ring::cpus.end()) *Ring::cpus.begin() = this->self.ring_fd;
    }

    this->registerFileDescriptor();

    this->registerCpu();

    this->registerFileDescriptors();
}

Ring::Ring(Ring &&ring) noexcept: self{ring.self} { ring.self = {}; }

auto Ring::operator=(Ring &&ring) noexcept -> Ring & {
    if (this != &ring) {
        this->self = ring.self;
        ring.self = {};
    }
    return *this;
}

auto Ring::registerFileDescriptor() -> void {
    int returnValue{io_uring_register_ring_fd(&this->self)};
    if (returnValue != 1)
        throw runtime_error("ring register fileDescriptor error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::registerCpu() -> void {
    lock_guard lockGuard{Ring::lock};

    cpu_set_t mask{};
    CPU_ZERO(&mask);
    auto result{find_if(Ring::cpus, [this](int i) { return i == this->self.ring_fd; })};
    if (result == Ring::cpus.end()) throw runtime_error("ring can not find self fileDescriptor");

    CPU_SET(std::distance(Ring::cpus.begin(), result), &mask);

    int returnValue{io_uring_register_iowq_aff(&this->self, sizeof(mask), &mask)};
    if (returnValue != 0)
        throw runtime_error("ring register cpu error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::registerFileDescriptors() -> void {
    rlimit limit{};
    int returnValue{getrlimit(RLIMIT_NOFILE, &limit)};
    if (returnValue != 0) throw runtime_error("ring get fileDescriptors limit error: " + string{std::strerror(errno)});

    returnValue = io_uring_register_files_sparse(&this->self, limit.rlim_cur);
    if (returnValue != 0)
        throw runtime_error("ring register fileDescriptors error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::registerBuffer(io_uring_buf_reg &reg) -> void {
    int returnValue{io_uring_register_buf_ring(&this->self, &reg, 0)};
    if (returnValue != 0)
        throw runtime_error("ring register buffer error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::unregisterBuffer(int bufferId) -> void {
    int returnValue{io_uring_unregister_buf_ring(&this->self, bufferId)};
    if (returnValue != 0)
        throw runtime_error("ring unregister buffer error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::getCompletion() -> io_uring_cqe * {
    io_uring_cqe *completion{nullptr};

    int returnValue{io_uring_wait_cqe(&this->self, &completion)};
    if (returnValue != 0)
        throw runtime_error("ring get completion error: " + string{std::strerror(std::abs(returnValue))});

    return completion;
}

auto Ring::consumeCompletion(io_uring_cqe *completion) -> void { io_uring_cqe_seen(&this->self, completion); }

auto Ring::getSubmission() -> io_uring_sqe * {
    io_uring_sqe *submission{io_uring_get_sqe(&this->self)};
    if (submission == nullptr) throw runtime_error("ring no available submission");

    return submission;
}

auto Ring::submit() -> void {
    int returnValue{io_uring_submit(&this->self)};
    if (returnValue < 0) throw runtime_error("ring submit error: " + string{std::strerror(std::abs(returnValue))});
}

Ring::~Ring() {
    io_uring_queue_exit(&this->self);

    {
        lock_guard lockGuard{Ring::lock};

        auto result{find_if(Ring::cpus, [this](int i) { return i == this->self.ring_fd; })};

        if (result != Ring::cpus.end())
            *result = -1;
        else
            Log::add(source_location::current(), Level::ERROR, "ring can not find fileDescriptor in cpus");
    }

    Ring::instance = false;
}
