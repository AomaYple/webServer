#include "EventLoop.h"

#include <cstring>

#include "Client.h"
#include "Completion.h"
#include "Http.h"
#include "Log.h"

using std::string, std::make_shared, std::source_location;

EventLoop::EventLoop(unsigned short port) : ring{make_shared<Ring>()}, buffer{ring}, server{port, ring} {}

auto EventLoop::loop() -> void {
    while (true) {
        Completion completion{this->ring};

        int result{completion.getResult()};
        void *data{completion.getData()};
        unsigned int flags{completion.getFlags()};

        if (data != nullptr) {
            if (data == &this->server) {
                if (result >= 0)
                    Client *client{new Client{result, this->ring, this->buffer}};
                else
                    Log::add(source_location::current(), Level::ERROR, std::strerror(std::abs(result)));

                if (!(flags | IORING_CQE_F_MORE)) this->server.accept(this->ring);
            } else {
                Client *client{static_cast<Client *>(data)};

                if (flags | IORING_CQE_F_BUFFER) {
                    if (result > 0) {
                        if (!(flags | IORING_CQE_F_MORE)) {
                            string request{this->buffer.update(flags >> IORING_CQE_BUFFER_SHIFT, result)};

                            auto response{Http::analysis(request)};

                            if (response.second) client->setKeepAlive(true);
                        }
                        client->send(std::move(response.first));
                    } else {
                        Log::add(source_location::current(), Level::ERROR,
                                 "client receive error: " + string{std::strerror(std::abs(result))});

                        delete client;
                    }
                } else {
                }
            }
        } else
            Log::add(source_location::current(), Level::ERROR, std::strerror(std::abs(result)));
    }
}
