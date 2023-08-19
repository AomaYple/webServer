#include "Database.h"

#include "../exception/Exception.h"
#include "../log/message.h"

using namespace std;

Database::Database(string_view host, string_view user, string_view password, string_view database, unsigned short port,
                   string_view unixSocket, unsigned long clientFlag)
    : connection{} {
    this->initialize();

    this->connect(host, user, password, database, port, unixSocket, clientFlag);
}

Database::Database(Database &&other) noexcept : connection{other.connection} { other.connection.host = nullptr; }

auto Database::initialize(source_location sourceLocation) -> void {
    if (!Database::instance) {
        Database::instance = true;
        Database::lock.lock();
    }

    if (mysql_init(&this->connection) == nullptr)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, "initialization failed")};

    Database::lock.unlock();
}

auto Database::connect(string_view host, string_view user, string_view password, string_view database,
                       unsigned short port, string_view unixSocket, unsigned long clientFlag,
                       source_location sourceLocation) -> void {
    if (mysql_real_connect(&this->connection, host.data(), user.data(), password.data(), database.data(), port,
                           unixSocket.data(), clientFlag) == nullptr)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, mysql_error(&this->connection))};
}

auto Database::consult(string_view statement) -> vector<vector<string>> {
    this->query(statement);

    MYSQL_RES *const consultResult{this->storeResult()};

    vector<vector<string>> results;

    if (consultResult != nullptr) {
        const unsigned int columnCount{Database::getColumnCount(consultResult)};

        for (MYSQL_ROW row{Database::getRow(consultResult)}; row != nullptr; row = Database::getRow(consultResult)) {
            vector<string> result;

            for (unsigned int i{0}; i < columnCount; ++i) result.emplace_back(row[i]);

            results.emplace_back(std::move(result));
        }

        Database::freeResult(consultResult);
    }

    return results;
}

auto Database::query(string_view statement, source_location sourceLocation) -> void {
    if (mysql_real_query(&this->connection, statement.data(), statement.size()) != 0)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, mysql_error(&this->connection))};
}

auto Database::storeResult(source_location sourceLocation) -> MYSQL_RES * {
    MYSQL_RES *const result{mysql_store_result(&this->connection)};

    if (result == nullptr) {
        const string_view error{mysql_error(&this->connection)};
        if (!error.empty())
            throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                             Level::FATAL, string{error})};
    }

    return result;
}

auto Database::getColumnCount(MYSQL_RES *result) noexcept -> unsigned int { return mysql_num_fields(result); }

auto Database::getRow(MYSQL_RES *result) noexcept -> MYSQL_ROW { return mysql_fetch_row(result); }

auto Database::freeResult(MYSQL_RES *result) noexcept -> void { mysql_free_result(result); }

Database::~Database() {
    if (this->connection.host != nullptr) mysql_close(&this->connection);
}

constinit thread_local bool Database::instance{false};
constinit std::mutex Database::lock;
