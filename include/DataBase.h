#pragma once

#include <source_location>
#include <string_view>
#include <vector>

#include <mysql/mysql.h>

class DataBase {
public:
    static auto query(std::string_view sql, std::source_location sourceLocation = std::source_location::current())
            -> std::vector<std::vector<std::string>>;

    DataBase(const DataBase &other) = delete;

    DataBase(DataBase &&other) = delete;

    ~DataBase();

private:
    DataBase();

    MYSQL self;

    static DataBase instance;
};
