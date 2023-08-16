#include "Server.h"

#include "../base/Submission.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../log/Log.h"
#include "../log/message.h"

#include <arpa/inet.h>

#include <cstring>

using std::array, std::shared_ptr, std::source_location;
using std::chrono::system_clock;
using std::this_thread::get_id;

Server::Server(uint_least16_t port, const shared_ptr<UserRing> &userRing)
    : fileDescriptor{Server::socket()}, userRing{userRing} {
    this->setSocketOption();

    this->bind(port);

    this->listen();
}

Server::Server(Server &&other) noexcept : fileDescriptor{other.fileDescriptor}, userRing{std::move(other.userRing)} {
    other.fileDescriptor = -1;
}

auto Server::socket(source_location sourceLocation) -> int {
    const int fileDescriptor{::socket(AF_INET, SOCK_STREAM, 0)};

    if (fileDescriptor == -1)
        throw Exception{
                message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL, std::strerror(errno))};

    return fileDescriptor;
}

auto Server::setSocketOption(source_location sourceLocation) const -> void {
    const int option{1};
    if (setsockopt(this->fileDescriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)) == -1)
        throw Exception{
                message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL, std::strerror(errno))};
}

auto Server::bind(std::uint_least16_t port, source_location sourceLocation) const -> void {
    sockaddr_in address{};
    const socklen_t addressLength{sizeof(address)};

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1)
        throw Exception{
                message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL, std::strerror(errno))};

    if (::bind(this->fileDescriptor, reinterpret_cast<sockaddr *>(&address), addressLength) == -1)
        throw Exception{
                message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL, std::strerror(errno))};
}

auto Server::listen(source_location sourceLocation) const -> void {
    if (::listen(this->fileDescriptor, SOMAXCONN) == -1)
        throw Exception{
                message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL, std::strerror(errno))};
}

auto Server::getFileDescriptor() const noexcept -> int { return this->fileDescriptor; }

auto Server::setFileDescriptor(int newFileDescriptor) noexcept -> void { this->fileDescriptor = newFileDescriptor; }

auto Server::accept() const -> void {
    const Submission submission{this->userRing->getSqe(), this->fileDescriptor, nullptr, nullptr, 0};

    const UserData userData{Type::ACCEPT, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<const __u64 &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);
}

Server::~Server() {
    if (this->fileDescriptor != -1) {
        try {
            this->cancel();

            this->close();
        } catch (const Exception &exception) { Log::produce(exception.what()); }
    }
}

auto Server::cancel() const -> void {
    const Submission submission{this->userRing->getSqe(), this->fileDescriptor, IORING_ASYNC_CANCEL_ALL};

    const UserData userData{Type::CANCEL, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<const __u64 &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);
}

auto Server::close() const -> void {
    const Submission submission{this->userRing->getSqe(), static_cast<unsigned int>(this->fileDescriptor)};

    const UserData userData{Type::CLOSE, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<const __u64 &>(userData));

    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);
}
