#include "Database.h"

#include "SqliteCompat.h"

#include <utility>

namespace acell
{
namespace storage
{

Database::Database()
    : db_(nullptr)
{
}

Database::Database(const std::string& path)
    : db_(nullptr)
{
    open(path);
}

Database::~Database()
{
    close();
}

Database::Database(Database&& other) noexcept
    : db_(other.db_)
{
    other.db_ = nullptr;
}

Database& Database::operator=(Database&& other) noexcept
{
    if(this != &other)
    {
        close();
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

void Database::open(const std::string& path)
{
    close();

    if(sqlite3_open(path.c_str(), &db_) != SQLITE_OK)
    {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "unknown sqlite open error";
        close();
        throw std::runtime_error("Failed to open database: " + msg);
    }

    exec("PRAGMA foreign_keys = ON;");
    exec("PRAGMA journal_mode = WAL;");
    exec("PRAGMA synchronous = NORMAL;");
}

void Database::close()
{
    if(db_)
    {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::isOpen() const
{
    return db_ != nullptr;
}

sqlite3* Database::handle() const
{
    return db_;
}

void Database::exec(const std::string& sql)
{
    char* err = nullptr;
    if(sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::string msg = err ? err : "sqlite exec error";
        sqlite3_free(err);
        throw std::runtime_error("Database exec failed: " + msg);
    }
}

void Database::beginTransaction()
{
    exec("BEGIN IMMEDIATE TRANSACTION;");
}

void Database::commit()
{
    exec("COMMIT;");
}

void Database::rollback()
{
    exec("ROLLBACK;");
}

std::int64_t Database::lastInsertRowId() const
{
    return static_cast<std::int64_t>(sqlite3_last_insert_rowid(db_));
}

Statement::Statement(sqlite3* db, const std::string& sql)
    : stmt_(nullptr)
{
    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK)
    {
        throwSqliteError(db, "prepare failed");
    }
}

Statement::~Statement()
{
    if(stmt_)
    {
        sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
}

Statement::Statement(Statement&& other) noexcept
    : stmt_(other.stmt_)
{
    other.stmt_ = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept
{
    if(this != &other)
    {
        if(stmt_)
        {
            sqlite3_finalize(stmt_);
        }
        stmt_ = other.stmt_;
        other.stmt_ = nullptr;
    }
    return *this;
}

sqlite3_stmt* Statement::get() const
{
    return stmt_;
}

void Statement::reset()
{
    sqlite3_reset(stmt_);
}

void Statement::clearBindings()
{
    sqlite3_clear_bindings(stmt_);
}

void throwSqliteError(sqlite3* db, const std::string& context)
{
    throw std::runtime_error(context + ": " + sqlite3_errmsg(db));
}

std::string columnText(sqlite3_stmt* stmt, int index)
{
    const unsigned char* txt = sqlite3_column_text(stmt, index);
    return txt ? reinterpret_cast<const char*>(txt) : "";
}

void bindInt64(sqlite3_stmt* stmt, int index, std::int64_t value)
{
    if(sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(value)) != SQLITE_OK)
    {
        throw std::runtime_error("sqlite bind int64 failed");
    }
}

void bindDouble(sqlite3_stmt* stmt, int index, double value)
{
    if(sqlite3_bind_double(stmt, index, value) != SQLITE_OK)
    {
        throw std::runtime_error("sqlite bind double failed");
    }
}

void bindText(sqlite3_stmt* stmt, int index, const std::string& value)
{
    if(sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
    {
        throw std::runtime_error("sqlite bind text failed");
    }
}

void bindNull(sqlite3_stmt* stmt, int index)
{
    if(sqlite3_bind_null(stmt, index) != SQLITE_OK)
    {
        throw std::runtime_error("sqlite bind null failed");
    }
}

bool columnIsNull(sqlite3_stmt* stmt, int index)
{
    return sqlite3_column_type(stmt, index) == SQLITE_NULL;
}

std::int64_t columnInt64(sqlite3_stmt* stmt, int index)
{
    return static_cast<std::int64_t>(sqlite3_column_int64(stmt, index));
}

double columnDouble(sqlite3_stmt* stmt, int index)
{
    return sqlite3_column_double(stmt, index);
}

} // namespace storage
} // namespace acell
