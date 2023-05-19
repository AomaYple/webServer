#include "EventLoop.h"

#include "Http.h"
#include "Log.h"

using std::string, std::shared_ptr, std::jthread, std::source_location;

EventLoop::EventLoop(unsigned short port, bool startThread) : server{port} {
    auto function{[this] {
        this->epoll.add(this->server.get(), EPOLLIN);
        this->epoll.add(this->timer.get(), EPOLLIN);

        while (true) {
            auto events{this->epoll.poll()};

            for (auto event{events.first.begin()}; event != events.first.begin() + events.second; ++event) {
                if (event->data.fd == this->timer.get())
                    this->timer.handleReadableEvent();
                else if (event->data.fd == this->server.get())
                    this->handleServerEvent();
                else
                    this->handleClientEvent(event->data.fd, event->events);
            }
        }
    }};

    if (startThread)
        this->work = jthread(function);
    else
        function();
}

EventLoop::EventLoop(EventLoop &&eventLoop) noexcept
    : server{std::move(eventLoop.server)},
      timer{std::move(eventLoop.timer)},
      epoll{std::move(eventLoop.epoll)},
      work{std::move(eventLoop.work)} {}

auto EventLoop::operator=(EventLoop &&eventLoop) noexcept -> EventLoop & {
    if (this != &eventLoop) {
        this->server = std::move(eventLoop.server);
        this->timer = std::move(eventLoop.timer);
        this->epoll = std::move(eventLoop.epoll);
        this->work = std::move(eventLoop.work);
    }
    return *this;
}

auto EventLoop::handleServerEvent() -> void {
    for (auto &client : this->server.accept()) {
        this->timer.add(client);

        this->epoll.add(client->get(), EPOLLET | EPOLLRDHUP | EPOLLIN | EPOLLOUT);
    }
}

auto EventLoop::handleClientEvent(int fileDescriptor, uint32_t event, source_location sourceLocation) -> void {
    auto client{this->timer.find(fileDescriptor)};

    if (client != nullptr) {
        if (event & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            this->timer.remove(client);
        else if (event & EPOLLIN && client->getEvent() == EPOLLIN) {
            this->handleClientReceivableEvent(client);

            if (event & EPOLLOUT && client->getEvent() == EPOLLOUT) this->handleClientSendableEvent(client);
        } else if (event & EPOLLOUT && client->getEvent() == EPOLLOUT) {
            this->handleClientSendableEvent(client);

            if (event & EPOLLIN && client->getEvent() == EPOLLIN) this->handleClientReceivableEvent(client);
        } else {
            Log::add(sourceLocation, Level::ERROR, string{client->getInformation()} + " unknown event");

            this->timer.remove(client);
        }
    }
}

auto EventLoop::handleClientReceivableEvent(shared_ptr<Client> &client) -> void {
    client->receive();

    if (client->getEvent() != 0) {
        this->timer.reset(client);

        auto response{Http::analysis(client->read())};
        if (!response.second) client->setKeepAlive(false);

        client->write(response.first);
    } else
        this->timer.remove(client);
}

auto EventLoop::handleClientSendableEvent(shared_ptr<Client> &client) -> void {
    client->send();

    if (client->getEvent() != 0)
        this->timer.reset(client);
    else
        this->timer.remove(client);
}
