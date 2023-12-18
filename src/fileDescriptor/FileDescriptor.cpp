#include "FileDescriptor.hpp"

#include "../ring/Submission.hpp"

#include <utility>

FileDescriptor::FileDescriptor(int fileDescriptor, std::shared_ptr<Ring> ring) noexcept
    : fileDescriptor{fileDescriptor}, ring{std::move(ring)} {}

FileDescriptor::FileDescriptor(FileDescriptor &&other) noexcept
    : fileDescriptor{std::exchange(other.fileDescriptor, -1)}, ring{std::move(other.ring)} {}

auto FileDescriptor::operator=(FileDescriptor &&other) noexcept -> FileDescriptor & {
    this->cancel();
    this->close();

    this->fileDescriptor = std::exchange(other.fileDescriptor, -1);
    this->ring = std::move(other.ring);

    return *this;
}

FileDescriptor::~FileDescriptor() {
    if (this->fileDescriptor != -1) {
        this->cancel();
        this->close();
    }
}

auto FileDescriptor::getFileDescriptor() const noexcept -> int { return this->fileDescriptor; }

auto FileDescriptor::getRing() const noexcept -> std::shared_ptr<Ring> { return this->ring; }

auto FileDescriptor::cancel() const -> void {
    const Submission submission{Event{Event::Type::cancel, this->fileDescriptor}, IOSQE_FIXED_FILE | IOSQE_IO_HARDLINK,
                                Submission::CancelParameters{IORING_ASYNC_CANCEL_ALL}};
    this->ring->submit(submission);
}

auto FileDescriptor::close() const -> void {
    const Submission submission{Event{Event::Type::close, this->fileDescriptor}, 0, Submission::CloseParameters{}};
    this->ring->submit(submission);
}
