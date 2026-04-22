#pragma once

#include "Database.h"
#include "StorageTypes.h"

#include <cstdint>
#include <vector>

namespace acell
{
namespace storage
{

class EventLogRepository
{
public:
    explicit EventLogRepository(Database& db);

    std::int64_t insert(const EventLogEntry& event);
    std::vector<EventLogEntry> loadRecent(std::int64_t cell_id, int limit);
    std::vector<EventLogEntry> loadByType(std::int64_t cell_id,
                                          const std::string& event_type,
                                          int limit);

private:
    Database& db_;
};

} // namespace storage
} // namespace acell
