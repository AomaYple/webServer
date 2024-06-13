#include "Mysql.hpp"

#include "../log/Exception.hpp"

auto Mysql::Deleter::operator()(MYSQL *const handle) const noexcept -> void { mysql_close(handle); }

auto Mysql::Result::Deleter::operator()(MYSQL_RES *const handle) const noexcept -> void { mysql_free_result(handle); }

Mysql::Result::Result(const std::unique_ptr<MYSQL, Mysql::Deleter> &mysqlHandle,
                      const std::source_location sourceLocation) :
    handle{[&mysqlHandle, sourceLocation] {
        MYSQL_RES *const handle{mysql_store_result(mysqlHandle.get())};
        if (handle == nullptr) {
            if (std::string error{mysql_error(mysqlHandle.get())}; !error.empty()) {
                throw Exception{
                    Log{Log::Level::error, std::move(error), sourceLocation}
                };
            }
        }

        return handle;
    }()} {}

auto Mysql::Result::get() const -> std::vector<std::vector<std::string>> {
    if (this->handle == nullptr) return {};

    std::vector<std::vector<std::string>> result;
    const unsigned int columnCount{this->getColumnCount()};
    for (std::vector row{this->getRow(columnCount)}; !row.empty(); row = this->getRow(columnCount))
        result.emplace_back(std::move(row));

    return result;
}

auto Mysql::Result::getColumnCount() const noexcept -> unsigned int { return mysql_num_fields(this->handle.get()); }

auto Mysql::Result::getRow(const unsigned int columnCount) const -> std::vector<std::string> {
    const char *const *const row{mysql_fetch_row(this->handle.get())};
    if (row == nullptr) return {};

    std::vector<std::string> result;
    for (unsigned int i{}; i < columnCount; ++i) result.emplace_back(row[i]);

    return result;
}

Mysql::Mysql(const std::source_location sourceLocation) :
    handle{[sourceLocation] {
        const std::lock_guard lockGuard{lock};

        MYSQL *const handle{mysql_init(nullptr)};
        if (handle == nullptr) {
            throw Exception{
                Log{Log::Level::fatal, "initialization of mysql handle failed", sourceLocation}
            };
        }

        return handle;
    }()} {}

auto Mysql::connect(const std::string_view host, const std::string_view user, const std::string_view password,
                    const std::string_view database, const unsigned int port, const std::string_view unixSocket,
                    const unsigned long clientFlag, const std::source_location sourceLocation) const -> void {
    if (mysql_real_connect(this->handle.get(), host.data(), user.data(), password.data(), database.data(), port,
                           unixSocket.data(), clientFlag) == nullptr) {
        throw Exception{
            Log{Log::Level::error, mysql_error(this->handle.get()), sourceLocation}
        };
    }
}

auto Mysql::inquire(const std::string_view statement) const -> std::vector<std::vector<std::string>> {
    this->query(statement);

    return Result{this->handle}.get();
}

auto Mysql::query(const std::string_view statement, const std::source_location sourceLocation) const -> void {
    if (mysql_real_query(this->handle.get(), statement.data(), statement.size()) != 0) {
        throw Exception{
            Log{Log::Level::error, mysql_error(this->handle.get()), sourceLocation}
        };
    }
}

constinit std::mutex Mysql::lock;
