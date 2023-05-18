#ifndef WEBSERVER_SERVER_H
#define WEBSERVER_SERVER_H

#include <memory>

#include "Client.h"

class Server {
 public:
  explicit Server(unsigned short port,
                  const std::source_location& sourceLocation =
                      std::source_location::current());

  Server(const Server& server) = delete;

  Server(Server&& server) noexcept;

  auto operator=(Server&& server) noexcept -> Server&;

  [[nodiscard]] auto accept(
      std::source_location sourceLocation = std::source_location::current())
      -> std::vector<std::shared_ptr<Client>>;

  [[nodiscard]] auto get() const -> int;

  ~Server();

 private:
  int socket, idleFileDescriptor;
};

#endif  //WEBSERVER_SERVER_H
