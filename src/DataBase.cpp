#include "DataBase.h"

#include "Log.h"

using std::string_view, std::string, std::vector, std::source_location, std::runtime_error;

auto DataBase::query(string_view sql, source_location sourceLocation) -> vector<vector<string>> {
    vector<vector<string>> result;

    if (mysql_real_query(&DataBase::instance.self, sql.data(), sql.size()) != 0)
        Log::add(sourceLocation, Level::WARN, "database query error");

    MYSQL_RES *res{mysql_store_result(&DataBase::instance.self)};

    MYSQL_ROW mysqlRow;

    unsigned int columns{mysql_num_fields(res)};

    while ((mysqlRow = mysql_fetch_row(res)) != nullptr) {
        vector<string> row;

        for (unsigned int i{0}; i < columns; ++i) row.emplace_back(mysqlRow[i]);

        result.emplace_back(std::move(row));
    }

    mysql_free_result(res);

    return result;
}

DataBase::DataBase() : self{} {
    if (mysql_init(&this->self) == nullptr) throw runtime_error("database initialize error");

    if (mysql_real_connect(&this->self, nullptr, "AomaYple", "38820233", "WebServer", 0, nullptr, 0) == nullptr)
        throw runtime_error("database connect error");
}

DataBase::~DataBase() { mysql_close(&this->self); }

DataBase DataBase::instance;
