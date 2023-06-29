#include "Ring.h"

#include <cstring>

#include <sys/resource.h>
#include <sys/sysinfo.h>

#include "Completion.h"
#include "Log.h"

using std::string, std::vector, std::array, std::function, std::ranges::find_if, std::mutex, std::lock_guard,
        std::runtime_error, std::source_location;

constexpr unsigned int entries{1024};

mutex Ring::lock{};
vector<int> Ring::values{vector<int>(get_nprocs(), -1)};

thread_local bool Ring::instance{false};

Ring::Ring() : self{} {
    if (Ring::instance) throw runtime_error("ring one thread one instance");
    Ring::instance = true;

    io_uring_params params{};
    params.flags = IORING_SETUP_SUBMIT_ALL | IORING_SETUP_CLAMP | IORING_SETUP_COOP_TASKRUN |
                   IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

    {
        lock_guard lockGuard{Ring::lock};

        auto result{find_if(Ring::values, [](int value) { return value != -1; })};

        if (result != Ring::values.end()) {
            params.flags |= IORING_SETUP_ATTACH_WQ;
            params.wq_fd = *result;
        }

        int returnValue{io_uring_queue_init_params(entries, &this->self, &params)};
        if (returnValue != 0)
            throw runtime_error("ring initialize error: " + string{std::strerror(std::abs(returnValue))});

        result = find_if(Ring::values, [](int value) { return value == -1; });
        if (result == Ring::values.end()) throw runtime_error("the number of rings has reached the maximum limit");
        *result = this->self.ring_fd;

        this->registerCpu();

        this->registerMaxWorks();
    }
    this->registerFileDescriptor();

    this->registerFileDescriptors();
}

Ring::Ring(Ring &&other) noexcept : self{other.self} { other.self.ring_fd = -1; }

auto Ring::operator=(Ring &&other) noexcept -> Ring & {
    if (this != &other) {
        this->self = other.self;
        other.self.ring_fd = -1;
    }
    return *this;
}

auto Ring::forEach(const function<auto(const Completion &)->void> &task) -> int {
    this->submitWait();

    unsigned int head;

    int number{0};

    io_uring_cqe *completion;
    io_uring_for_each_cqe(&this->self, head, completion) {
        task(Completion{completion});

        ++number;
    }

    return number;
}

auto Ring::get() -> io_uring & { return this->self; }

Ring::~Ring() {
    if (this->self.ring_fd != -1) {
        Ring::instance = false;

        io_uring_queue_exit(&this->self);

        {
            lock_guard lockGuard{Ring::lock};

            auto result{find_if(Ring::values, [this](int value) { return value == this->self.ring_fd; })};
            if (result == Ring::values.end())
                Log::add(source_location::current(), Level::ERROR, "ring can not find self file descriptor");

            *result = -1;
        }
    }
}

auto Ring::registerCpu() -> void {
    cpu_set_t cpuSet{};

    auto result{find_if(Ring::values, [this](int value) { return value == this->self.ring_fd; })};
    if (result == Ring::values.end()) throw runtime_error("ring can not find self file descriptor");

    CPU_SET(std::distance(Ring::values.begin(), result), &cpuSet);

    int returnValue{io_uring_register_iowq_aff(&this->self, sizeof(cpuSet), &cpuSet)};
    if (returnValue != 0)
        throw runtime_error("ring register cpu error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::registerMaxWorks() -> void {
    array<unsigned int, 2> value{0, static_cast<unsigned int>(get_nprocs())};

    int returnValue{io_uring_register_iowq_max_workers(&this->self, value.data())};
    if (returnValue != 0)
        throw runtime_error("ring register max works error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::registerFileDescriptor() -> void {
    int returnValue{io_uring_register_ring_fd(&this->self)};
    if (returnValue != 1)
        throw runtime_error("ring register file descriptor error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::registerFileDescriptors() -> void {
    rlimit limit{};
    if (getrlimit(RLIMIT_NOFILE, &limit) == -1)
        throw runtime_error("ring get file descriptors limit error: " + string{std::strerror(errno)});

    int returnValue{io_uring_register_files_sparse(&this->self, limit.rlim_cur)};
    if (returnValue != 0)
        throw runtime_error("ring register file descriptors error: " + string{std::strerror(std::abs(returnValue))});

    this->allocFileDescriptorRange(1, limit.rlim_cur - 1);
}

auto Ring::allocFileDescriptorRange(unsigned int start, unsigned int length) -> void {
    int returnValue{io_uring_register_file_alloc_range(&this->self, start, length)};
    if (returnValue != 0)
        throw runtime_error("ring alloc file descriptor range error :" + string{std::strerror(std::abs(returnValue))});
}

auto Ring::submitWait() -> void {
    int returnValue{io_uring_submit_and_wait(&this->self, 1)};
    if (returnValue < 0) throw runtime_error("ring submit wait error: " + string{std::strerror(std::abs(returnValue))});
}
