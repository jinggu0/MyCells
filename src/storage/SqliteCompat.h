#pragma once

#if defined(__has_include)
#    if __has_include(<sqlite3.h>)
#        include <sqlite3.h>
#    else
#        define ACELL_STORAGE_DECLARE_SQLITE 1
#    endif
#else
#    define ACELL_STORAGE_DECLARE_SQLITE 1
#endif

#if defined(ACELL_STORAGE_DECLARE_SQLITE)

struct sqlite3;
struct sqlite3_stmt;

using sqlite3_int64 = long long;
using sqlite3_destructor_type = void (*)(void*);

constexpr int SQLITE_OK = 0;
constexpr int SQLITE_NULL = 5;
constexpr int SQLITE_ROW = 100;
constexpr int SQLITE_DONE = 101;

#define SQLITE_TRANSIENT (reinterpret_cast<sqlite3_destructor_type>(-1))

extern "C"
{
int sqlite3_open(const char* filename, sqlite3** ppDb);
int sqlite3_close(sqlite3* db);
int sqlite3_exec(sqlite3* db,
                 const char* sql,
                 int (*callback)(void*, int, char**, char**),
                 void* firstArg,
                 char** errmsg);
void sqlite3_free(void* value);
const char* sqlite3_errmsg(sqlite3* db);
sqlite3_int64 sqlite3_last_insert_rowid(sqlite3* db);

int sqlite3_prepare_v2(sqlite3* db,
                       const char* sql,
                       int nByte,
                       sqlite3_stmt** ppStmt,
                       const char** pzTail);
int sqlite3_finalize(sqlite3_stmt* stmt);
int sqlite3_reset(sqlite3_stmt* stmt);
int sqlite3_step(sqlite3_stmt* stmt);
int sqlite3_clear_bindings(sqlite3_stmt* stmt);

int sqlite3_bind_int64(sqlite3_stmt* stmt, int index, sqlite3_int64 value);
int sqlite3_bind_double(sqlite3_stmt* stmt, int index, double value);
int sqlite3_bind_text(sqlite3_stmt* stmt,
                      int index,
                      const char* value,
                      int n,
                      sqlite3_destructor_type destructor);
int sqlite3_bind_null(sqlite3_stmt* stmt, int index);

int sqlite3_column_type(sqlite3_stmt* stmt, int index);
sqlite3_int64 sqlite3_column_int64(sqlite3_stmt* stmt, int index);
double sqlite3_column_double(sqlite3_stmt* stmt, int index);
const unsigned char* sqlite3_column_text(sqlite3_stmt* stmt, int index);
}

#endif
