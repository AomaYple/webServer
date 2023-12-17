#include "FileDescriptor.hpp"

#include "../ring/Submission.hpp"

FileDescriptor::FileDescriptor(int fileDescriptor, std::shared_ptr<Ring> ring) noexcept
    : fileDescriptor{fileDescriptor}, ring{std::move(ring)} {}

auto FileDescriptor::getFileDescriptor() const noexcept -> int { return this->fileDescriptor; }

auto FileDescriptor::cancel() const -> void {
    const Submission submission{Event{Event::Type::cancel, this->fileDescriptor}, IOSQE_FIXED_FILE,
                                Submission::CancelParameters{IORING_ASYNC_CANCEL_ALL}};

    this->ring->submit(submission);
}

auto FileDescriptor::close() const -> void {
    const Submission submission{Event{Event::Type::close, this->fileDescriptor}, 0, Submission::CloseParameters{}};

    this->ring->submit(submission);
}

auto FileDescriptor::getRing() const noexcept -> std::shared_ptr<Ring> { return this->ring; }
