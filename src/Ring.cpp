#include "Ring.h"

#include <sys/resource.h>
#include <sys/sysinfo.h>

#include <cstring>

#include "Completion.h"
#include "Log.h"

using std::string, std::vector, std::array, std::function, std::runtime_error, std::mutex, std::lock_guard,
    std::source_location, std::ranges::find_if;

mutex Ring::lock{};
vector<int> Ring::values{vector<int>(get_nprocs(), -1)};

constexpr unsigned int ringEntries{1024};

thread_local bool Ring::instance{false};

Ring::Ring() : self{} {
    if (Ring::instance) throw runtime_error("ring one thread one instance");
    Ring::instance = true;

    io_uring_params params{};
    params.flags = IORING_SETUP_CLAMP | IORING_SETUP_SUBMIT_ALL | IORING_SETUP_COOP_TASKRUN |
                   IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

    {
        lock_guard lockGuard{Ring::lock};

        auto result{find_if(Ring::values, [](int value) { return value != -1; })};
        if (result != Ring::values.end()) {
            params.flags |= IORING_SETUP_ATTACH_WQ;
            params.wq_fd = *result;
        }

        int returnValue{io_uring_queue_init_params(ringEntries, &this->self, &params)};
        if (returnValue != 0)
            throw runtime_error("ring initialize error: " + string{std::strerror(std::abs(returnValue))});

        result = find_if(Ring::values, [](int value) { return value == -1; });
        if (result == Ring::values.end()) throw runtime_error("the number of rings has reached the upper limit");

        *result = this->self.ring_fd;

        this->registerCpu();
    }

    this->registerMaxWorks();

    this->registerRingFileDescriptor();

    this->registerSparseFileDescriptors();
}

Ring::Ring(Ring &&other) noexcept : self{other.self} { other.self.ring_fd = -1; }

auto Ring::operator=(Ring &&other) noexcept -> Ring & {
    if (this != &other) {
        this->self = other.self;
        other.self.ring_fd = -1;
    }
    return *this;
}

auto Ring::updateFileDescriptor(int fileDescriptor) -> void {
    int returnValue{io_uring_register_files_update(&this->self, fileDescriptor, &fileDescriptor, 1)};
    if (returnValue < 0)
        throw runtime_error("ring update fileDescriptor error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::setupBufferRing(unsigned int number, int id) -> io_uring_buf_ring * {
    int returnValue;
    io_uring_buf_ring *bufferRing{io_uring_setup_buf_ring(&this->self, number, id, 0, &returnValue)};

    if (bufferRing == nullptr)
        throw runtime_error("ring setup bufferRing error: " + string{std::strerror(std::abs(returnValue))});

    return bufferRing;
}

auto Ring::freeBufferRing(io_uring_buf_ring *bufferRing, unsigned int number, int id) -> void {
    int returnValue{io_uring_free_buf_ring(&this->self, bufferRing, number, id)};

    if (returnValue < 0)
        throw runtime_error("ring free bufferRing error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::forEach(const function<auto(const Completion &completion, source_location sourceLocation)->void> &task)
    -> int {
    this->submitWait();

    int completionNumber{0};

    unsigned int head;
    io_uring_cqe *completion;

    io_uring_for_each_cqe(&this->self, head, completion) {
        ++completionNumber;
        task(Completion{completion}, source_location::current());
    }

    return completionNumber;
}

auto Ring::getSubmission() -> io_uring_sqe * {
    io_uring_sqe *submission{io_uring_get_sqe(&this->self)};

    if (submission == nullptr) throw runtime_error("ring no available submission");

    return submission;
}

auto Ring::advanceCompletionBufferRing(io_uring_buf_ring *bufferRing, int completionNumber, int bufferRingNumber)
    -> void {
    __io_uring_buf_ring_cq_advance(&this->self, bufferRing, completionNumber, bufferRingNumber);
}

Ring::~Ring() {
    if (this->self.ring_fd != -1) {
        io_uring_queue_exit(&this->self);

        {
            lock_guard lockGuard{Ring::lock};

            auto result{find_if(Ring::values, [this](int value) { return value == this->self.ring_fd; })};
            if (result == Ring::values.end())
                Log::add(source_location::current(), Level::ERROR, "ring can not find self fileDescriptor");

            *result = -1;
        }

        Ring::instance = false;
    }
}

auto Ring::registerCpu() -> void {
    cpu_set_t mask{};
    CPU_ZERO(&mask);

    auto result{find_if(Ring::values, [this](int value) { return value == this->self.ring_fd; })};
    if (result == Ring::values.end()) throw runtime_error("ring can not find self fileDescriptor");

    CPU_SET(std::distance(Ring::values.begin(), result), &mask);

    int returnValue{io_uring_register_iowq_aff(&this->self, sizeof(mask), &mask)};
    if (returnValue != 0)
        throw runtime_error("ring register cpu error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::registerMaxWorks() -> void {
    array<unsigned int, 2> value{0, static_cast<unsigned int>(get_nprocs())};

    int returnValue{io_uring_register_iowq_max_workers(&this->self, value.data())};
    if (returnValue != 0)
        throw runtime_error("ring register max works error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::registerRingFileDescriptor() -> void {
    int returnValue{io_uring_register_ring_fd(&this->self)};
    if (returnValue != 1)
        throw runtime_error("ring register self fileDescriptor error: " + string{std::strerror(std::abs(returnValue))});
}

auto Ring::registerSparseFileDescriptors() -> void {
    rlimit limit{};
    int returnValue{getrlimit(RLIMIT_NOFILE, &limit)};
    if (returnValue != 0) throw runtime_error("ring get fileDescriptors limit error: " + string{std::strerror(errno)});

    returnValue = io_uring_register_files_sparse(&this->self, limit.rlim_cur);
    if (returnValue != 0)
        throw runtime_error("ring register sparse fileDescriptors error: " +
                            string{std::strerror(std::abs(returnValue))});
}

auto Ring::submitWait() -> void {
    int returnValue{io_uring_submit_and_wait(&this->self, 1)};
    if (returnValue < 0) throw runtime_error("ring submit wait error: " + string{std::strerror(std::abs(returnValue))});
}
