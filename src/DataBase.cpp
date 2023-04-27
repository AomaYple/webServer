#include "DataBase.h"
#include "Log.h"

using std::string, std::vector, std::source_location;

DataBase::DataBase(const string &user, const string &password, const string &database) : self({}) {
    if (mysql_init(&this->self) == nullptr)
        Log::add(source_location::current(), Level::ERROR, "DataBase init error");

    if (mysql_real_connect(&this->self, nullptr, user.c_str(), password.c_str(), database.c_str(), 0, nullptr, 0) == nullptr)
        Log::add(source_location::current(), Level::ERROR, "DataBase connect error");
}

DataBase::DataBase(DataBase &&dataBase) noexcept : self(dataBase.self) {
    dataBase.self = {};
}

auto DataBase::operator=(DataBase &&dataBase) noexcept -> DataBase & {
    if (this != &dataBase) {
        this->self = dataBase.self;
        dataBase.self = {};
    }
    return *this;
}

auto DataBase::query(const string &statement) -> vector<vector<string>> {
    vector<vector<string>> result;

    if (mysql_real_query(&this->self, statement.c_str(), statement.size()) != 0)
        Log::add(source_location::current(), Level::ERROR, "DataBase query error");

    MYSQL_RES *storeResult {mysql_store_result(&this->self)};

    unsigned int columns {mysql_num_fields(storeResult)};

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(storeResult)) != nullptr) {
        vector<string> rowData(columns);

        for (unsigned int i {0}; i < columns; ++i)
            rowData[i] = row[i];

        result.emplace_back(std::move(rowData));
    }

    mysql_free_result(storeResult);

    return result;
}

DataBase::~DataBase() {
    mysql_close(&this->self);
}
