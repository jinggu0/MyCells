#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace acell
{
namespace storage
{

class Database
{
public:
    Database();
    explicit Database(const std::string& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    Database(Database&& other) noexcept;
    Database& operator=(Database&& other) noexcept;

    void open(const std::string& path);
    void close();

    bool isOpen() const;
    sqlite3* handle() const;

    void exec(const std::string& sql);

    void beginTransaction();
    void commit();
    void rollback();

    std::int64_t lastInsertRowId() const;

private:
    sqlite3* db_;
};

class Statement
{
public:
    Statement(sqlite3* db, const std::string& sql);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    sqlite3_stmt* get() const;

    void reset();
    void clearBindings();

private:
    sqlite3_stmt* stmt_;
};

void throwSqliteError(sqlite3* db, const std::string& context);
std::string columnText(sqlite3_stmt* stmt, int index);

void bindInt64(sqlite3_stmt* stmt, int index, std::int64_t value);
void bindDouble(sqlite3_stmt* stmt, int index, double value);
void bindText(sqlite3_stmt* stmt, int index, const std::string& value);
void bindNull(sqlite3_stmt* stmt, int index);

bool columnIsNull(sqlite3_stmt* stmt, int index);
std::int64_t columnInt64(sqlite3_stmt* stmt, int index);
double columnDouble(sqlite3_stmt* stmt, int index);

} // namespace storage
} // namespace acell
