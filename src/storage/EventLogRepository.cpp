#include "EventLogRepository.h"

#include "SqliteCompat.h"

namespace acell
{
namespace storage
{

namespace
{

EventLogEntry readEvent(sqlite3_stmt* stmt)
{
    EventLogEntry e;
    e.id = columnInt64(stmt, 0);
    e.cell_id = columnInt64(stmt, 1);

    if(!columnIsNull(stmt, 2))
    {
        e.session_id = columnInt64(stmt, 2);
    }

    e.event_type = columnText(stmt, 3);
    e.event_severity = columnText(stmt, 4);
    e.event_time = columnText(stmt, 5);
    e.sim_time_seconds = columnDouble(stmt, 6);
    e.title = columnText(stmt, 7);
    e.description = columnText(stmt, 8);
    e.payload_json = columnText(stmt, 9);

    if(!columnIsNull(stmt, 10)) e.mass_before = columnDouble(stmt, 10);
    if(!columnIsNull(stmt, 11)) e.mass_after = columnDouble(stmt, 11);
    if(!columnIsNull(stmt, 12)) e.energy_before = columnDouble(stmt, 12);
    if(!columnIsNull(stmt, 13)) e.energy_after = columnDouble(stmt, 13);
    if(!columnIsNull(stmt, 14)) e.health_before = columnDouble(stmt, 14);
    if(!columnIsNull(stmt, 15)) e.health_after = columnDouble(stmt, 15);
    if(!columnIsNull(stmt, 16)) e.stress_before = columnDouble(stmt, 16);
    if(!columnIsNull(stmt, 17)) e.stress_after = columnDouble(stmt, 17);

    e.created_at = columnText(stmt, 18);
    return e;
}

void bindOptionalDouble(sqlite3_stmt* stmt, int idx, const std::optional<double>& v)
{
    if(v.has_value()) bindDouble(stmt, idx, *v);
    else bindNull(stmt, idx);
}

void bindOptionalInt64(sqlite3_stmt* stmt, int idx, const std::optional<std::int64_t>& v)
{
    if(v.has_value()) bindInt64(stmt, idx, *v);
    else bindNull(stmt, idx);
}

} // namespace

EventLogRepository::EventLogRepository(Database& db)
    : db_(db)
{
}

std::int64_t EventLogRepository::insert(const EventLogEntry& event)
{
    static const char* sql =
        "INSERT INTO event_logs ("
        "cell_id, session_id, event_type, event_severity, event_time, sim_time_seconds, "
        "title, description, payload_json, "
        "mass_before, mass_after, energy_before, energy_after, "
        "health_before, health_after, stress_before, stress_after, "
        "created_at"
        ") VALUES ("
        "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?"
        ");";

    Statement stmt(db_.handle(), sql);

    bindInt64(stmt.get(), 1, event.cell_id);
    bindOptionalInt64(stmt.get(), 2, event.session_id);
    bindText(stmt.get(), 3, event.event_type);
    bindText(stmt.get(), 4, event.event_severity);
    bindText(stmt.get(), 5, event.event_time);
    bindDouble(stmt.get(), 6, event.sim_time_seconds);
    bindText(stmt.get(), 7, event.title);
    bindText(stmt.get(), 8, event.description);
    bindText(stmt.get(), 9, event.payload_json);
    bindOptionalDouble(stmt.get(), 10, event.mass_before);
    bindOptionalDouble(stmt.get(), 11, event.mass_after);
    bindOptionalDouble(stmt.get(), 12, event.energy_before);
    bindOptionalDouble(stmt.get(), 13, event.energy_after);
    bindOptionalDouble(stmt.get(), 14, event.health_before);
    bindOptionalDouble(stmt.get(), 15, event.health_after);
    bindOptionalDouble(stmt.get(), 16, event.stress_before);
    bindOptionalDouble(stmt.get(), 17, event.stress_after);
    bindText(stmt.get(), 18, event.created_at);

    if(sqlite3_step(stmt.get()) != SQLITE_DONE)
    {
        throwSqliteError(db_.handle(), "EventLogRepository::insert");
    }

    return db_.lastInsertRowId();
}

std::vector<EventLogEntry> EventLogRepository::loadRecent(std::int64_t cell_id, int limit)
{
    static const char* sql =
        "SELECT "
        "id, cell_id, session_id, event_type, event_severity, event_time, sim_time_seconds, "
        "title, description, payload_json, "
        "mass_before, mass_after, energy_before, energy_after, "
        "health_before, health_after, stress_before, stress_after, "
        "created_at "
        "FROM event_logs "
        "WHERE cell_id = ? "
        "ORDER BY event_time DESC, id DESC "
        "LIMIT ?;";

    Statement stmt(db_.handle(), sql);
    bindInt64(stmt.get(), 1, cell_id);
    bindInt64(stmt.get(), 2, limit);

    std::vector<EventLogEntry> out;
    while(true)
    {
        const int rc = sqlite3_step(stmt.get());
        if(rc == SQLITE_DONE)
        {
            break;
        }
        if(rc != SQLITE_ROW)
        {
            throwSqliteError(db_.handle(), "EventLogRepository::loadRecent");
        }
        out.push_back(readEvent(stmt.get()));
    }
    return out;
}

std::vector<EventLogEntry> EventLogRepository::loadByType(std::int64_t cell_id,
                                                          const std::string& event_type,
                                                          int limit)
{
    static const char* sql =
        "SELECT "
        "id, cell_id, session_id, event_type, event_severity, event_time, sim_time_seconds, "
        "title, description, payload_json, "
        "mass_before, mass_after, energy_before, energy_after, "
        "health_before, health_after, stress_before, stress_after, "
        "created_at "
        "FROM event_logs "
        "WHERE cell_id = ? AND event_type = ? "
        "ORDER BY event_time DESC, id DESC "
        "LIMIT ?;";

    Statement stmt(db_.handle(), sql);
    bindInt64(stmt.get(), 1, cell_id);
    bindText(stmt.get(), 2, event_type);
    bindInt64(stmt.get(), 3, limit);

    std::vector<EventLogEntry> out;
    while(true)
    {
        const int rc = sqlite3_step(stmt.get());
        if(rc == SQLITE_DONE)
        {
            break;
        }
        if(rc != SQLITE_ROW)
        {
            throwSqliteError(db_.handle(), "EventLogRepository::loadByType");
        }
        out.push_back(readEvent(stmt.get()));
    }
    return out;
}

} // namespace storage
} // namespace acell
