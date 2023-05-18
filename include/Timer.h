#ifndef WEBSERVER_TIMER_H
#define WEBSERVER_TIMER_H

#include <array>
#include <memory>
#include <unordered_map>

#include "Client.h"

class Timer {
 public:
  explicit Timer(const std::source_location& sourceLocation =
                     std::source_location::current());

  Timer(const Timer& timer) = delete;

  Timer(Timer&& timer) noexcept;

  auto operator=(Timer&& timer) noexcept -> Timer&;

  auto add(const std::shared_ptr<Client>& client) -> void;

  auto find(int clientFileDescriptor) -> std::shared_ptr<Client>;

  auto reset(const std::shared_ptr<Client>& client) -> void;

  auto remove(const std::shared_ptr<Client>& client) -> void;

  auto handleReadableEvent(const std::source_location& sourceLocation =
                               std::source_location::current()) -> void;

  auto get() const -> int;

  ~Timer();

 private:
  int fileDescriptor;
  unsigned short now;
  std::array<std::unordered_map<int, std::shared_ptr<Client>>, 61> wheel;
  std::unordered_map<int, unsigned short> location;
};

#endif  //WEBSERVER_TIMER_H
