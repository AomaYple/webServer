#include "Database.hpp"

#include "../log/Exception.hpp"

Database::Database(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
                   unsigned short port, std::string_view unixSocket, unsigned int clientFlag)
    : connection{} {
    this->initialize();

    this->connect(host, user, password, database, port, unixSocket, clientFlag);
}

Database::Database(Database &&other) noexcept : connection{other.connection} { other.connection.host = nullptr; }

auto Database::operator=(Database &&other) noexcept -> Database & {
    if (this != &other) {
        this->destroy();

        this->connection = other.connection;
        other.connection.host = nullptr;
    }

    return *this;
}

Database::~Database() { this->destroy(); }

auto Database::consult(std::string_view statement) -> std::vector<std::vector<std::string>> {
    std::vector<std::vector<std::string>> results;

    this->query(statement);

    MYSQL_RES *const consultResult{this->getResult()};

    if (consultResult == nullptr) return results;

    const unsigned int columnCount{Database::getColumnCount(*consultResult)};

    for (std::vector<std::string> row{Database::getRow(*consultResult, columnCount)}; !row.empty();
         row = Database::getRow(*consultResult, columnCount))
        results.emplace_back(std::move(row));

    Database::freeResult(*consultResult);

    return results;
}

auto Database::destroy() noexcept -> void {
    if (this->connection.host != nullptr) mysql_close(&this->connection);
}

auto Database::initialize(std::source_location sourceLocation) -> void {
    const std::lock_guard lockGuard{Database::lock};

    if (mysql_init(&this->connection) == nullptr)
        throw Exception{Log{Log::Level::fatal, "initialization of database failed", sourceLocation}};
}

auto Database::connect(std::string_view host, std::string_view user, std::string_view password,
                       std::string_view database, unsigned short port, std::string_view unixSocket,
                       unsigned int clientFlag, std::source_location sourceLocation) -> void {
    if (mysql_real_connect(&this->connection, host.data(), user.data(), password.data(), database.data(), port,
                           unixSocket.data(), clientFlag) == nullptr)
        throw Exception{Log{Log::Level::fatal, mysql_error(&this->connection), sourceLocation}};
}

auto Database::query(std::string_view statement, std::source_location sourceLocation) -> void {
    if (mysql_real_query(&this->connection, statement.data(), statement.size()) != 0)
        throw Exception{Log{Log::Level::fatal, mysql_error(&this->connection), sourceLocation}};
}

auto Database::getResult(std::source_location sourceLocation) -> MYSQL_RES * {
    MYSQL_RES *const result{mysql_store_result(&this->connection)};

    if (result == nullptr) {
        const std::string_view error{mysql_error(&this->connection)};
        if (!error.empty()) throw Exception{Log{Log::Level::fatal, std::string{error}, sourceLocation}};
    }

    return result;
}

auto Database::getColumnCount(MYSQL_RES &result) noexcept -> unsigned int { return mysql_num_fields(&result); }

auto Database::getRow(MYSQL_RES &result, unsigned int columnCount) -> std::vector<std::string> {
    std::vector<std::string> row;

    const char *const *const rowPointer{mysql_fetch_row(&result)};
    for (unsigned int i{0}; rowPointer != nullptr && i < columnCount; ++i) row.emplace_back(rowPointer[i]);

    return row;
}

auto Database::freeResult(MYSQL_RES &result) noexcept -> void { mysql_free_result(&result); }

constinit std::mutex Database::lock;
