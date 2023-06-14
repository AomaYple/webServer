#include "EventLoop.h"

#include "Http.h"
#include "Log.h"

using std::string, std::span, std::shared_ptr, std::source_location;

EventLoop::EventLoop(unsigned short port) : server{port} {}

EventLoop::EventLoop(EventLoop &&other) noexcept
    : server{std::move(other.server)}, timer{std::move(other.timer)}, epoll{std::move(other.epoll)} {}

auto EventLoop::operator=(EventLoop &&other) noexcept -> EventLoop & {
    if (this != &other) {
        this->server = std::move(other.server);
        this->timer = std::move(other.timer);
        this->epoll = std::move(other.epoll);
    }
    return *this;
}

auto EventLoop::operator()() -> void {
    this->epoll.add(this->server.get(), EPOLLIN);
    this->epoll.add(this->timer.get(), EPOLLIN);

    while (true) {
        span<epoll_event> epollEvents{this->epoll.poll()};

        for (const epoll_event &epollEvent: epollEvents) {
            int fileDescriptor{epollEvent.data.fd};
            unsigned int event{epollEvent.events};

            if (fileDescriptor == this->timer.get()) this->timer.handleTimeout();
            else if (fileDescriptor == this->server.get())
                this->handleServer();
            else
                this->handleClient(fileDescriptor, event);
        }
    }
}

auto EventLoop::handleServer() -> void {
    for (auto &client: this->server.accept()) {
        this->epoll.add(client->get(), EPOLLET | EPOLLRDHUP | EPOLLIN | EPOLLOUT);

        this->timer.add(client);
    }
}

auto EventLoop::handleClient(int socket, unsigned int event, source_location sourceLocation) -> void {
    shared_ptr<Client> client{this->timer.find(socket)};

    if (event & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) this->timer.remove(client);
    else if (event & EPOLLIN && client->getEvent() == EPOLLIN) {
        this->handleClientReceive(client);

        if (event & EPOLLOUT && client->getEvent() == EPOLLOUT) this->handleClientSend(client);
    } else if (event & EPOLLOUT && client->getEvent() == EPOLLOUT) {
        this->handleClientSend(client);

        if (event & EPOLLIN && client->getEvent() == EPOLLIN) this->handleClientReceive(client);
    } else {
        Log::add(sourceLocation, Level::ERROR, string{client->getInformation()} + " unknown event");

        this->timer.remove(client);
    }
}

auto EventLoop::handleClientReceive(shared_ptr<Client> &client) -> void {
    client->receive();

    if (client->getEvent() != 0) {
        this->timer.reset(client);

        auto response{Http::analysis(client->read())};
        if (!response.second) client->setKeepAlive(false);

        client->write(response.first);
    } else
        this->timer.remove(client);
}

auto EventLoop::handleClientSend(shared_ptr<Client> &client) -> void {
    client->send();

    if (client->getEvent() != 0) this->timer.reset(client);
    else
        this->timer.remove(client);
}
