#include "Server.h"

#include <arpa/inet.h>

#include <cstring>

#include "../base/Submission.h"
#include "../exception/Exception.h"
#include "../log/Log.h"
#include "UserData.h"

using std::array;
using std::shared_ptr;
using std::source_location;

Server::Server(unsigned short port, const shared_ptr<UserRing> &userRing)
    : fileDescriptor{Server::socket()}, userRing{userRing} {
    this->setSocketOption();

    this->bind(port);

    this->listen();

    this->registerFileDescriptor();
}

auto Server::accept() -> void {
    Submission submission{this->userRing->getSqe()};

    UserData userData{Type::ACCEPT, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<unsigned long long &>(userData));

    submission.accept(this->fileDescriptor, nullptr, nullptr, 0);

    submission.setFlags(IOSQE_FIXED_FILE);
}

Server::~Server() {
    if (this->fileDescriptor != -1) {
        try {
            this->cancel();

            this->close();

            this->unregisterFileDescriptor();
        } catch (Exception &exception) { Log::produce(exception.getMessage()); }
    }
}

auto Server::socket(source_location sourceLocation) -> int {
    int fileDescriptor{::socket(AF_INET, SOCK_STREAM, 0)};

    if (fileDescriptor == -1) throw Exception{sourceLocation, Level::FATAL, std::strerror(errno)};

    return fileDescriptor;
}

auto Server::setSocketOption(source_location sourceLocation) const -> void {
    int option{1};
    if (setsockopt(this->fileDescriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)) == -1)
        throw Exception{sourceLocation, Level::FATAL, std::strerror(errno)};
}

auto Server::bind(unsigned short port, source_location sourceLocation) const -> void {
    sockaddr_in address{};
    socklen_t addressLength{sizeof(address)};

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1)
        throw Exception{sourceLocation, Level::FATAL, std::strerror(errno)};

    if (::bind(this->fileDescriptor, reinterpret_cast<sockaddr *>(&address), addressLength) == -1)
        throw Exception{sourceLocation, Level::FATAL, std::strerror(errno)};
}

auto Server::listen(source_location sourceLocation) const -> void {
    if (::listen(this->fileDescriptor, SOMAXCONN) == -1)
        throw Exception{sourceLocation, Level::FATAL, std::strerror(errno)};
}

auto Server::registerFileDescriptor() -> void {
    this->userRing->allocateFileDescriptorRange(1, getFileDescriptorLimit() - 1);

    array<int, 1> fileDescriptors{this->fileDescriptor};
    this->userRing->updateFileDescriptors(0, fileDescriptors);

    this->fileDescriptor = 0;
}

auto Server::cancel() -> void {
    Submission submission{this->userRing->getSqe()};

    UserData event{Type::CANCEL, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<unsigned long long &>(event));

    submission.cancel(this->fileDescriptor, IORING_ASYNC_CANCEL_ALL);

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);
}

auto Server::close() -> void {
    Submission submission{this->userRing->getSqe()};

    UserData event{Type::CLOSE, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<unsigned long long &>(event));

    submission.close(this->fileDescriptor);

    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);
}

auto Server::unregisterFileDescriptor() -> void {
    this->userRing->allocateFileDescriptorRange(0, getFileDescriptorLimit());
}
