#pragma once

#include <sys/socket.h>

#include <span>
#include <variant>

class Submission {
public:
    struct Accept {
        sockaddr *address;
        socklen_t *addressLength;
        int flags;
    };

    struct Read {
        std::span<std::byte> buffer;
        unsigned long offset;
    };

    struct Receive {
        std::span<std::byte> buffer;
        int flags;
        int ringBufferId;
    };

    struct Send {
        std::span<const std::byte> buffer;
        int flags;
        unsigned int zeroCopyFlags;
    };

    struct Close {};

    Submission(int fileDescriptor, std::variant<Accept, Read, Receive, Send, Close> &&parameter, unsigned int flags);

    Submission(const Submission &);

    auto operator=(const Submission &) -> Submission &;

    Submission(Submission &&) noexcept;

    auto operator=(Submission &&) noexcept -> Submission &;

    ~Submission();

    [[nodiscard]] auto getFileDescriptor() const noexcept -> int;

    [[nodiscard]] auto getParameter() const noexcept -> const std::variant<Accept, Read, Receive, Send, Close> &;

    [[nodiscard]] auto getFlags() const noexcept -> unsigned int;

    [[nodiscard]] auto getUserData() const noexcept -> void *;

private:
    auto destroy() const noexcept -> void;

    int fileDescriptor;
    std::variant<Accept, Read, Receive, Send, Close> parameter;
    unsigned int flags;
    void *userData{static_cast<void *>(new unsigned long)};
};
