#pragma once

#include "../coroutine/Awaiter.hpp"

class FileDescriptor {
public:
    explicit FileDescriptor(int fileDescriptor) noexcept;

    FileDescriptor(const FileDescriptor &) = delete;

    auto operator=(const FileDescriptor &) = delete;

    FileDescriptor(FileDescriptor &&) noexcept = default;

    auto operator=(FileDescriptor &&) noexcept -> FileDescriptor & = default;

    ~FileDescriptor() = default;

    [[nodiscard]] auto getFileDescriptor() const noexcept -> int;

    [[nodiscard]] auto close() noexcept -> Awaiter &;

protected:
    auto getAwaiter() noexcept -> Awaiter &;

private:
    int fileDescriptor;
    Awaiter awaiter;
};
