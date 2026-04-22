#include "UserActionRepository.h"

#include "SqliteCompat.h"

namespace acell
{
namespace storage
{

namespace
{

UserActionEntry readUserAction(sqlite3_stmt* stmt)
{
    UserActionEntry a;
    a.id = columnInt64(stmt, 0);
    a.cell_id = columnInt64(stmt, 1);

    if(!columnIsNull(stmt, 2))
    {
        a.session_id = columnInt64(stmt, 2);
    }

    a.action_type = columnText(stmt, 3);
    a.action_time = columnText(stmt, 4);
    a.payload_json = columnText(stmt, 5);
    a.created_at = columnText(stmt, 6);
    return a;
}

void bindOptionalInt64(sqlite3_stmt* stmt, int idx, const std::optional<std::int64_t>& v)
{
    if(v.has_value()) bindInt64(stmt, idx, *v);
    else bindNull(stmt, idx);
}

} // namespace

UserActionRepository::UserActionRepository(Database& db)
    : db_(db)
{
}

std::int64_t UserActionRepository::insert(const UserActionEntry& action)
{
    static const char* sql =
        "INSERT INTO user_actions ("
        "cell_id, session_id, action_type, action_time, payload_json, created_at"
        ") VALUES (?, ?, ?, ?, ?, ?);";

    Statement stmt(db_.handle(), sql);
    bindInt64(stmt.get(), 1, action.cell_id);
    bindOptionalInt64(stmt.get(), 2, action.session_id);
    bindText(stmt.get(), 3, action.action_type);
    bindText(stmt.get(), 4, action.action_time);
    bindText(stmt.get(), 5, action.payload_json);
    bindText(stmt.get(), 6, action.created_at);

    if(sqlite3_step(stmt.get()) != SQLITE_DONE)
    {
        throwSqliteError(db_.handle(), "UserActionRepository::insert");
    }

    return db_.lastInsertRowId();
}

std::vector<UserActionEntry> UserActionRepository::loadRecent(std::int64_t cell_id, int limit)
{
    static const char* sql =
        "SELECT "
        "id, cell_id, session_id, action_type, action_time, payload_json, created_at "
        "FROM user_actions "
        "WHERE cell_id = ? "
        "ORDER BY action_time DESC, id DESC "
        "LIMIT ?;";

    Statement stmt(db_.handle(), sql);
    bindInt64(stmt.get(), 1, cell_id);
    bindInt64(stmt.get(), 2, limit);

    std::vector<UserActionEntry> out;
    while(true)
    {
        const int rc = sqlite3_step(stmt.get());
        if(rc == SQLITE_DONE)
        {
            break;
        }
        if(rc != SQLITE_ROW)
        {
            throwSqliteError(db_.handle(), "UserActionRepository::loadRecent");
        }
        out.push_back(readUserAction(stmt.get()));
    }

    return out;
}

} // namespace storage
} // namespace acell