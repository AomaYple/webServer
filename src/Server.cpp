#include "Server.h"

#include <arpa/inet.h>

#include <cstring>

#include "Event.h"
#include "Log.h"
#include "Submission.h"

using std::string, std::array, std::shared_ptr, std::runtime_error, std::source_location;

Server::Server(unsigned short port, const shared_ptr<UserRing> &userRing)
    : socket{::socket(AF_INET, SOCK_STREAM, 0)}, userRing{userRing} {
    if (this->socket == -1) throw runtime_error("server create error: " + string{std::strerror(errno)});

    this->setSocketOption();

    this->bind(port);

    this->listen();

    this->userRing->allocateFileDescriptorRange(1, getFileDescriptorLimit() - 1);

    array<int, 1> fileDescriptor{this->socket};
    this->userRing->updateFileDescriptors(0, fileDescriptor);

    this->socket = 0;
}

Server::Server(Server &&other) noexcept : socket{other.socket}, userRing{std::move(other.userRing)} {
    other.socket = -1;
}

auto Server::operator=(Server &&other) noexcept -> Server & {
    if (this != &other) {
        this->socket = other.socket;
        this->userRing = std::move(other.userRing);
        other.socket = -1;
    }
    return *this;
}

auto Server::accept() -> void {
    Submission submission{this->userRing->getSubmission()};

    Event event{Type::ACCEPT, this->socket};
    submission.setUserData(reinterpret_cast<unsigned long long &>(event));

    submission.accept(this->socket, nullptr, nullptr, 0);

    submission.setFlags(IOSQE_FIXED_FILE);
}

Server::~Server() {
    if (this->socket != -1) {
        try {
            this->cancel();

            this->close();

            this->userRing->allocateFileDescriptorRange(0, getFileDescriptorLimit());
        } catch (const runtime_error &runtimeError) {
            Log::produce(source_location::current(), Level::ERROR, runtimeError.what());
        }
    }
}

auto Server::setSocketOption() const -> void {
    int option{1};
    if (setsockopt(this->socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)) == -1)
        throw runtime_error("server set socket option error: " + string{std::strerror(errno)});
}

auto Server::bind(unsigned short port) const -> void {
    sockaddr_in address{};
    socklen_t addressLength{sizeof(address)};

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1)
        throw runtime_error("server translate ipAddress error: " + string{std::strerror(errno)});

    if (::bind(this->socket, reinterpret_cast<sockaddr *>(&address), addressLength) == -1)
        throw runtime_error("server bind error: " + string{std::strerror(errno)});
}

auto Server::listen() const -> void {
    if (::listen(this->socket, SOMAXCONN) == -1)
        throw runtime_error("server listen error: " + string{std::strerror(errno)});
}

auto Server::cancel() -> void {
    Submission submission{this->userRing->getSubmission()};

    Event event{Type::CANCEL, this->socket};
    submission.setUserData(reinterpret_cast<unsigned long long &>(event));

    submission.cancel(this->socket, IORING_ASYNC_CANCEL_ALL);

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);
}

auto Server::close() -> void {
    Submission submission{this->userRing->getSubmission()};

    Event event{Type::CLOSE, this->socket};
    submission.setUserData(reinterpret_cast<unsigned long long &>(event));

    submission.close(this->socket);

    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);
}
