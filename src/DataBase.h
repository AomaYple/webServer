#pragma once

#include <mysql/mysql.h>

#include <string>
#include <vector>

class DataBase {
public:
    DataBase(const std::string &user, const std::string &password, const std::string &database);

    DataBase(const DataBase &dataBase) = delete;

    DataBase(DataBase &&dataBase) noexcept;

    auto operator=(DataBase &&dataBase) noexcept -> DataBase &;

    auto query(const std::string &statement) -> std::vector<std::vector<std::string>>;

    ~DataBase();
private:
    MYSQL self;
};
