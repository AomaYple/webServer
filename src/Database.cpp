#include "Database.h"

#include <stdexcept>

using std::runtime_error;
using std::string, std::string_view;
using std::vector;

Database::Database(string_view host, string_view user, string_view password, string_view database, unsigned int port,
                   string_view unixSocket, unsigned long clientFlag)
    : connection{Database::initialize()} {
    this->connect(host, user, password, database, port, unixSocket, clientFlag);
}

Database::Database(Database &&other) noexcept : connection{other.connection} { other.connection = nullptr; }

auto Database::operator=(Database &&other) noexcept -> Database & {
    if (this != &other) {
        this->connection = other.connection;
        other.connection = nullptr;
    }
    return *this;
}

auto Database::insert(string &&table, vector<string> &&filedS, vector<string> &&values) -> void {
    string statement{Database::insertCombine(std::move(table), std::move(filedS), std::move(values))};

    this->insertQuery(std::move(statement));
}

auto Database::consult(vector<string> &&filedS, vector<std::string> &&tables, string &&condition, string &&limit,
                       string &&offset) -> vector<vector<string>> {
    string statement{Database::consultCombine(std::move(filedS), std::move(tables), std::move(condition),
                                              std::move(limit), std::move(offset))};

    return this->consultQuery(std::move(statement));
}

Database::~Database() {
    if (this->connection != nullptr) mysql_close(this->connection);
}

auto Database::initialize() -> MYSQL * {
    MYSQL *tempConnection{mysql_init(nullptr)};
    if (tempConnection == nullptr) throw runtime_error("database failed to initialize connection");

    return tempConnection;
}

auto Database::connect(string_view host, string_view user, string_view password, string_view database,
                       unsigned int port, string_view unixSocket, unsigned long clientFlag) -> void {
    if (mysql_real_connect(connection, host.empty() ? nullptr : host.data(), user.data(), password.data(),
                           database.data(), port, unixSocket.empty() ? nullptr : unixSocket.data(),
                           clientFlag) == nullptr)
        throw runtime_error("database connect error: " + string{mysql_error(this->connection)});
}

auto Database::query(string &&statement) -> void {
    if (mysql_query(this->connection, statement.data()) != 0)
        throw runtime_error("database query error: " + string{mysql_error(this->connection)});
}

auto Database::storeResult() -> MYSQL_RES * {
    MYSQL_RES *result{mysql_store_result(this->connection)};

    if (result == nullptr) {
        const char *error{mysql_error(this->connection)};
        if (error != nullptr)
            throw runtime_error("database store result error: " + string{mysql_error(this->connection)});
    }

    return result;
}

auto Database::getColumnCount(MYSQL_RES *result) -> unsigned int { return mysql_num_fields(result); }

auto Database::getRow(MYSQL_RES *result) -> MYSQL_ROW { return mysql_fetch_row(result); }

auto Database::freeResult(MYSQL_RES *result) -> void { mysql_free_result(result); }

auto Database::insertCombine(string &&table, vector<string> &&filedS, vector<string> &&values) -> string {
    string statement{"insert into " + table + " ("};

    for (auto &filed: filedS) statement += std::move(filed) + ',';
    statement.back() = ')';

    statement += " values (";

    for (auto &value: values) statement += std::move(value) + ',';
    statement.back() = ')';

    statement += ';';

    return statement;
}

auto Database::insertQuery(string &&statement) -> void { this->query(std::move(statement)); }

auto Database::consultCombine(vector<string> &&filedS, vector<std::string> &&tables, string &&condition, string &&limit,
                              string &&offset) -> string {
    string statement{"select "};

    for (auto &field: filedS) statement += std::move(field) + ',';
    statement.back() = ' ';

    statement += "from ";

    for (auto &table: tables) statement += std::move(table) + ',';
    statement.back() = ' ';

    statement += std::move(condition) + ' ' + std::move(limit) + ' ' + std::move(offset) + ';';

    return statement;
}
auto Database::consultQuery(string &&statement) -> vector<vector<string>> {
    this->query(std::move(statement));

    vector<vector<string>> results;

    MYSQL_RES *mysqlRes{this->storeResult()};

    unsigned int columnCount{Database::getColumnCount(mysqlRes)};

    for (MYSQL_ROW mysqlRow; (mysqlRow = Database::getRow(mysqlRes)) != nullptr;) {
        vector<string> result;

        for (unsigned int i{0}; i < columnCount; ++i) result.emplace_back(mysqlRow[i]);

        results.emplace_back(std::move(result));
    }

    Database::freeResult(mysqlRes);

    return results;
}
