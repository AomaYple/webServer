#include "Server.h"

#include "../base/Submission.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../log/Log.h"
#include "../log/message.h"

#include <arpa/inet.h>

#include <cstring>

using namespace std;

auto Server::create(unsigned short port) -> unsigned int {
    const unsigned int fileDescriptor{Server::socket()};

    Server::setSocketOption(fileDescriptor);

    Server::bind(fileDescriptor, port);

    Server::listen(fileDescriptor);

    return fileDescriptor;
}

Server::Server(unsigned int fileDescriptorIndex, const shared_ptr<UserRing> &userRing)
    : fileDescriptorIndex{fileDescriptorIndex}, userRing{userRing} {}

Server::Server(Server &&other) noexcept
    : fileDescriptorIndex{other.fileDescriptorIndex}, userRing{std::move(other.userRing)} {}

auto Server::socket(source_location sourceLocation) -> unsigned int {
    const int fileDescriptor{::socket(AF_INET, SOCK_STREAM, 0)};

    if (fileDescriptor == -1)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(errno))};

    return fileDescriptor;
}

auto Server::setSocketOption(unsigned int fileDescriptor, source_location sourceLocation) -> void {
    const int option{1};
    if (setsockopt(static_cast<int>(fileDescriptor), SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option,
                   sizeof(option)) == -1)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(errno))};
}

auto Server::bind(unsigned int fileDescriptor, unsigned short port, source_location sourceLocation) -> void {
    sockaddr_in address{};
    const socklen_t addressLength{sizeof(address)};

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(errno))};

    if (::bind(static_cast<int>(fileDescriptor), reinterpret_cast<sockaddr *>(&address), addressLength) == -1)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(errno))};
}

auto Server::listen(unsigned int fileDescriptor, source_location sourceLocation) -> void {
    if (::listen(static_cast<int>(fileDescriptor), SOMAXCONN) == -1)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(errno))};
}

auto Server::accept() const -> void {
    const Submission submission{this->userRing->getSqe(), this->fileDescriptorIndex, nullptr, nullptr, 0};

    const UserData userData{Type::ACCEPT, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);
}

Server::~Server() {
    if (this->userRing != nullptr) {
        try {
            this->cancel();

            this->close();
        } catch (const Exception &exception) { Log::produce(exception.what()); }
    }
}

auto Server::cancel() const -> void {
    const Submission submission{this->userRing->getSqe(), this->fileDescriptorIndex, IORING_ASYNC_CANCEL_ALL};

    const UserData userData{Type::CANCEL, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);
}

auto Server::close() const -> void {
    const Submission submission{this->userRing->getSqe(), this->fileDescriptorIndex};

    const UserData userData{Type::CLOSE, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);
}
