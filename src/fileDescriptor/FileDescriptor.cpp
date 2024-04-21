#include "FileDescriptor.hpp"

#include <liburing/io_uring.h>

FileDescriptor::FileDescriptor(int fileDescriptor) noexcept : fileDescriptor{fileDescriptor} {}

auto FileDescriptor::getFileDescriptor() const noexcept -> int { return this->fileDescriptor; }

auto FileDescriptor::cancel() const noexcept -> Awaiter {
    Awaiter awaiter;
    awaiter.setSubmission(Submission{this->fileDescriptor,
                                     Submission::Cancel{IORING_ASYNC_CANCEL_ALL | IORING_ASYNC_CANCEL_FD_FIXED}, 0, 0});

    return awaiter;
}

auto FileDescriptor::close() const noexcept -> Awaiter {
    Awaiter awaiter;
    awaiter.setSubmission(Submission{this->fileDescriptor, Submission::Close{}, 0, 0});

    return awaiter;
}
