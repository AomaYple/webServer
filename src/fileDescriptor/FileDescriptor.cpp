#include "FileDescriptor.hpp"

#include "../ring/Submission.hpp"

FileDescriptor::FileDescriptor(int fileDescriptor) noexcept : fileDescriptor{fileDescriptor} {}

auto FileDescriptor::getFileDescriptor() const noexcept -> int { return this->fileDescriptor; }

auto FileDescriptor::getAwaiter() noexcept -> Awaiter & { return this->awaiter; }

auto FileDescriptor::close() noexcept -> Awaiter & {
    this->awaiter.submit(std::make_shared<Submission>(this->fileDescriptor, Submission::Close{}, 0));

    return this->awaiter;
}
