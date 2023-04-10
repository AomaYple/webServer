#include "EventLoop.h"
#include "Http.h"
#include "Log.h"

using std::string, std::shared_ptr, std::jthread, std::source_location;

EventLoop::EventLoop(unsigned short port, bool startThread) : server(port) {
    auto function {[this] {
        this->epoll.add(this->timeEpoll.get(), EPOLLET | EPOLLIN);
        this->epoll.add(this->socketEpoll.get(), EPOLLET | EPOLLIN);

        this->timeEpoll.add(this->timer.get(), EPOLLET | EPOLLIN);
        this->socketEpoll.add(this->server.get(), EPOLLIN);

        while (true) {
            auto events {this->epoll.poll()};

            for (auto event {events.first.begin()}; event != events.first.begin() + events.second; ++event) {
                if (event->data.fd == this->timeEpoll.get())
                    this->handleTimeEvent();
                else
                    this->handleSocketEvent();
            }
        }
    }};

    if (startThread)
        this->work = jthread(function);
    else
        function();
}


EventLoop::EventLoop(EventLoop &&eventLoop) noexcept : server(std::move(eventLoop.server)), epoll(std::move(eventLoop.epoll)),
        timeEpoll(std::move(eventLoop.timeEpoll)), socketEpoll(std::move(eventLoop.socketEpoll)), timer(std::move(eventLoop.timer)),
        table(std::move(eventLoop.table)), work(std::move(eventLoop.work)) {}

auto EventLoop::operator=(EventLoop &&eventLoop) noexcept -> EventLoop & {
    if (this != &eventLoop) {
        this->server = std::move(eventLoop.server);
        this->epoll = std::move(eventLoop.epoll);
        this->timeEpoll = std::move(eventLoop.timeEpoll);
        this->socketEpoll = std::move(eventLoop.socketEpoll);
        this->timer = std::move(eventLoop.timer);
        this->table = std::move(eventLoop.table);
        this->work = std::move(eventLoop.work);
    }
    return *this;
}

auto EventLoop::handleTimeEvent() -> void {
    auto events {this->timeEpoll.poll(false)};

    for (auto event {events.first.begin()}; event != events.first.begin() + events.second; ++event) {
        auto fileDescriptors {this->timer.handleRead()};

        for (int fileDescriptor : fileDescriptors)
            this->table.erase(fileDescriptor);
    }
}

auto EventLoop::handleSocketEvent() -> void {
    auto events {this->socketEpoll.poll(false)};

    for (auto event {events.first.begin()}; event != events.first.begin() + events.second; ++event) {
        if (event->data.fd == this->server.get())
            this->handleServerEvent();
        else
            this->handleClientEvent(event->data.fd, event->events);
    }
}

auto EventLoop::handleServerEvent() -> void {
    for (auto &client : this->server.accept()) {
        this->table.emplace(client->get(), client);

        this->socketEpoll.add(client->get(), EPOLLET | EPOLLRDHUP | EPOLLIN);
    }
}

auto EventLoop::handleClientEvent(int fileDescriptor, uint32_t event) -> void {
    shared_ptr<Client> client {this->table[fileDescriptor]};

    if (event & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
        this->removeClient(client);
    else if (event & EPOLLIN)
        this->handleClientReceivableEvent(client);
    else if (event & EPOLLOUT)
        this->handleClientSendableEvent(client);
    else
        this->handleUnknownEvent(client);
}

auto EventLoop::removeClient(shared_ptr<Client> &client) -> void {
    if (client->getExpire() != 0)
        this->timer.remove(client);

    this->table.erase(client->get());
}

auto EventLoop::handleClientReceivableEvent(shared_ptr<Client> &client) -> void {
    uint32_t event {client->receive()};

    if (event != 0) {
        auto response {Http::analysis(client->read())};

        client->write(response.first);

        if (response.second && client->getExpire() == 0) {
            client->setExpire(60);

            this->timer.add(client);
        } else if (response.second && client->getExpire() != 0)
            this->timer.reset(client);

        this->socketEpoll.mod(client->get(), event);
    } else
        this->removeClient(client);
}

auto EventLoop::handleClientSendableEvent(shared_ptr<Client> &client) -> void {
    uint32_t event {client->send()};

    if (event != 0) {
        if (client->getExpire() != 0)
            this->timer.reset(client);

        this->socketEpoll.mod(client->get(), event);
    } else
        this->removeClient(client);
}

auto EventLoop::handleUnknownEvent(shared_ptr<Client> &client) -> void {
    this->removeClient(client);

    Log::add(source_location::current(), Level::ERROR,  string(client->getInformation()) + " occur unknown event");
}
