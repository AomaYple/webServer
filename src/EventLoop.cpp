#include "EventLoop.h"

#include <cstring>

#include "Completion.h"
#include "Event.h"
#include "Log.h"

using std::string, std::make_shared, std::source_location;

EventLoop::EventLoop(unsigned short port) : ring{make_shared<Ring>()}, bufferRing{ring}, server{port, ring} {}

EventLoop::EventLoop(EventLoop &&other) noexcept
    : ring{std::move(other.ring)},
      bufferRing{std::move(other.bufferRing)},
      server{std::move(other.server)},
      clients{std::move(other.clients)} {}

auto EventLoop::operator=(EventLoop &&other) noexcept -> EventLoop & {
    if (this != &other) {
        this->ring = std::move(other.ring);
        this->bufferRing = std::move(other.bufferRing);
        this->server = std::move(other.server);
        this->clients = std::move(other.clients);
    }
    return *this;
}

auto EventLoop::loop() -> void {
    while (true) {
        int completionNumber{this->ring->forEach([this](const Completion &completion, source_location sourceLocation) {
            int result{completion.getResult()};

            unsigned long long data{completion.getData()};
            Event event{reinterpret_cast<Event &>(data)};

            unsigned int flags{completion.getFlags()};

            switch (event.type) {
                case Type::ACCEPT:
                    this->handleAccept(result, flags);

                    break;
                case Type::UPDATE_TIMEOUT:
                    Log::add(sourceLocation, Level::ERROR,
                             "client update timer error: " + string{std::strerror(std::abs(result))});

                    break;
                case Type::RECEIVE:
                    this->handleReceive(result, event.fileDescriptor, flags);

                    break;
                case Type::TIMEOUT:
                    Log::add(
                        sourceLocation, Level::INFO,
                        "timeout " + std::to_string(event.fileDescriptor) + string{std::strerror(std::abs(result))});

                    break;
                case Type::REMOVE_TIMEOUT:
                    Log::add(sourceLocation, Level::ERROR,
                             "client remove timer error: " + string{std::strerror(std::abs(result))});

                    break;
                case Type::CANCEL_FILE_DESCRIPTOR:
                    Log::add(sourceLocation, Level::ERROR,
                             "client cancel all request error: " + string{std::strerror(std::abs(result))});

                    break;
                case Type::CLOSE:
                    Log::add(sourceLocation, Level::ERROR,
                             "client close error: " + string{std::strerror(std::abs(result))});

                    break;
            }
        })};
        this->bufferRing.advanceCompletionBufferRing(completionNumber);
    }
}

auto EventLoop::handleAccept(int result, unsigned int flags, source_location sourceLocation) -> void {
    if (result >= 0)
        this->clients.emplace(result, Client{result, this->ring, this->bufferRing});
    else
        Log::add(sourceLocation, Level::WARN, "server accept error: " + string{std::strerror(std::abs(result))});

    if (!(flags | IORING_CQE_F_MORE)) this->server.accept();
}

auto EventLoop::handleReceive(int result, int fileDescriptor, unsigned int flags, source_location sourceLocation)
    -> void {
    if (result > 0) {
        Client &client{this->clients.at(fileDescriptor)};

        client.updateTimeout();

        string data{this->bufferRing.getData(flags >> IORING_CQE_BUFFER_SHIFT, result)};

        client.write(std::move(data));

        if (!(flags | IORING_CQE_F_SOCK_NONEMPTY)) Log::add(sourceLocation, Level::INFO, client.read());

        if (!(flags | IORING_CQE_F_MORE)) client.receive(this->bufferRing);
    } else {
        Log::add(sourceLocation, Level::WARN, std::string{std::strerror(std::abs(result))});

        this->clients.erase(fileDescriptor);
    }
}
