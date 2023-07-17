#include "Server.h"

#include <arpa/inet.h>

#include <cstring>

#include "Log.h"
#include "Submission.h"
#include "UserData.h"

using std::array, std::string;
using std::runtime_error;
using std::shared_ptr;
using std::source_location;

Server::Server(unsigned short port, const shared_ptr<UserRing> &userRing) : fileDescriptor{}, userRing{userRing} {
    this->socket();

    this->setSocketOption();

    this->bind(port);

    this->listen();

    this->registerFileDescriptor();
}

Server::Server(Server &&other) noexcept : fileDescriptor{other.fileDescriptor}, userRing{std::move(other.userRing)} {
    other.fileDescriptor = -1;
}

auto Server::operator=(Server &&other) noexcept -> Server & {
    if (this != &other) {
        this->fileDescriptor = other.fileDescriptor;
        this->userRing = std::move(other.userRing);
        other.fileDescriptor = -1;
    }
    return *this;
}

auto Server::accept() -> void {
    Submission submission{this->userRing->getSubmission()};

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
        } catch (const runtime_error &runtimeError) {
            Log::produce(source_location::current(), Level::ERROR, runtimeError.what());
        }
    }
}

auto Server::socket() -> void {
    this->fileDescriptor = ::socket(AF_INET, SOCK_STREAM, 0);

    if (this->fileDescriptor == -1)
        throw runtime_error("server create file descriptor error: " + string{std::strerror(errno)});
}

auto Server::setSocketOption() const -> void {
    int option{1};
    if (setsockopt(this->fileDescriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)) == -1)
        throw runtime_error("server set file descriptor option error: " + string{std::strerror(errno)});
}

auto Server::bind(unsigned short port) const -> void {
    sockaddr_in address{};
    socklen_t addressLength{sizeof(address)};

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1)
        throw runtime_error("server translate ip address error: " + string{std::strerror(errno)});

    if (::bind(this->fileDescriptor, reinterpret_cast<sockaddr *>(&address), addressLength) == -1)
        throw runtime_error("server bind error: " + string{std::strerror(errno)});
}

auto Server::listen() const -> void {
    if (::listen(this->fileDescriptor, SOMAXCONN) == -1)
        throw runtime_error("server listen error: " + string{std::strerror(errno)});
}

auto Server::registerFileDescriptor() -> void {
    this->userRing->allocateFileDescriptorRange(1, getFileDescriptorLimit() - 1);

    array<int, 1> fileDescriptors{this->fileDescriptor};
    this->userRing->updateFileDescriptors(0, fileDescriptors);

    this->fileDescriptor = 0;
}

auto Server::cancel() -> void {
    Submission submission{this->userRing->getSubmission()};

    UserData event{Type::CANCEL, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<unsigned long long &>(event));

    submission.cancel(this->fileDescriptor, IORING_ASYNC_CANCEL_ALL);

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);
}

auto Server::close() -> void {
    Submission submission{this->userRing->getSubmission()};

    UserData event{Type::CLOSE, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<unsigned long long &>(event));

    submission.close(this->fileDescriptor);

    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);
}

auto Server::unregisterFileDescriptor() -> void {
    this->userRing->allocateFileDescriptorRange(0, getFileDescriptorLimit());
}
