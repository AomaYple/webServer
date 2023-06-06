#include "EventLoop.h"

#include <cstring>

#include "Client.h"
#include "Completion.h"
#include "Log.h"
#include "Submission.h"

using std::string, std::make_shared, std::source_location;

EventLoop::EventLoop(unsigned short port)
    : ring{make_shared<Ring>()}, buffer{ring}, server{port, *ring}, task{[this] {
          while (true) {
              auto result{this->ring->forEach([this](const Completion& completion) -> bool {
                  bool useBuffer{false};

                  int result{completion.getResult()};
                  void* data{completion.getData()};
                  unsigned int flags{completion.getFlags()};

                  if (data == &this->server) {
                      if (result >= 0)
                          Client* client{new Client{result, *this->ring, this->buffer}};
                      else
                          Log::add(source_location::current(), Level::ERROR,
                                   "server accept error: " + string{std::strerror(std::abs(result))});

                      if (!(flags | IORING_CQE_F_MORE)) this->server.accept(*this->ring);
                  } else {
                  }

                  return useBuffer;
              })};

              this->buffer.advanceBufferCompletion(result.first);
              this->ring->advanceCompletion(result.second);
          }
      }} {}

auto EventLoop::loop() -> void { this->task(); }
