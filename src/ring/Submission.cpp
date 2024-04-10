#include "Submission.hpp"

#include <utility>

Submission::Submission(int fileDescriptor, std::variant<Accept, Read, Receive, Send, Close> &&parameter,
                       unsigned int flags)
    : fileDescriptor{fileDescriptor}, parameter{parameter}, flags{flags} {}

Submission::Submission(const Submission &other)
    : fileDescriptor{other.fileDescriptor}, parameter{other.parameter}, flags{other.flags},
      userData{new unsigned long{*static_cast<unsigned long *>(other.userData)}} {}

auto Submission::operator=(const Submission &other) -> Submission & {
    if (this != &other) {
        this->destroy();

        this->fileDescriptor = other.fileDescriptor;
        this->parameter = other.parameter;
        this->flags = other.flags;
        this->userData = new unsigned long{*static_cast<unsigned long *>(other.userData)};
    }

    return *this;
}

Submission::Submission(Submission &&other) noexcept
    : fileDescriptor{std::exchange(other.fileDescriptor, 0)},
      parameter{std::exchange(other.parameter, std::variant<Accept, Read, Receive, Send, Close>{})},
      flags{std::exchange(other.flags, 0)}, userData{std::exchange(other.userData, nullptr)} {}

auto Submission::operator=(Submission &&other) noexcept -> Submission & {
    if (this != &other) {
        this->destroy();

        this->fileDescriptor = std::exchange(other.fileDescriptor, 0);
        this->parameter = std::exchange(other.parameter, std::variant<Accept, Read, Receive, Send, Close>{});
        this->flags = std::exchange(other.flags, 0);
        this->userData = std::exchange(other.userData, nullptr);
    }

    return *this;
}

Submission::~Submission() { this->destroy(); }

auto Submission::getFileDescriptor() const noexcept -> int { return this->fileDescriptor; }

auto Submission::getParameter() const noexcept -> const std::variant<Accept, Read, Receive, Send, Close> & {
    return this->parameter;
}

auto Submission::getFlags() const noexcept -> unsigned int { return this->flags; }

auto Submission::getUserData() const noexcept -> void * { return this->userData; }

auto Submission::destroy() const noexcept -> void { delete static_cast<unsigned long *>(userData); }