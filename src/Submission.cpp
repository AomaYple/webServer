#include "Submission.h"

#include <stdexcept>

using std::runtime_error;

Submission::Submission(io_uring_sqe *submission) : self{submission} {}

Submission::Submission(Submission &&other) noexcept : self{other.self} {}

auto Submission::operator=(Submission &&other) noexcept -> Submission & {
    if (this != &other) this->self = other.self;
    return *this;
}

auto Submission::setData(unsigned long long int data) -> void { io_uring_sqe_set_data64(this->self, data); }

auto Submission::setFlags(unsigned int flags) -> void { io_uring_sqe_set_flags(this->self, flags); }

auto Submission::accept(int socket, sockaddr *address, socklen_t *addressLength, int flags) -> void {
    io_uring_prep_multishot_accept_direct(this->self, socket, address, addressLength, flags);
}
