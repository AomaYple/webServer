#include "FileDescriptor.hpp"

#include <linux/io_uring.h>

FileDescriptor::FileDescriptor(const int fileDescriptor) noexcept : fileDescriptor{fileDescriptor} {}

auto FileDescriptor::getFileDescriptor() const noexcept -> int { return this->fileDescriptor; }

auto FileDescriptor::cancel() const noexcept -> Awaiter {
    return Awaiter{
        Submission{this->fileDescriptor, 0, 0, 0,
                   Submission::Cancel{IORING_ASYNC_CANCEL_ALL | IORING_ASYNC_CANCEL_FD_FIXED}}
    };
}

auto FileDescriptor::close() const noexcept -> Awaiter {
    return Awaiter{
        Submission{this->fileDescriptor, 0, 0, 0, Submission::Close{}}
    };
}
