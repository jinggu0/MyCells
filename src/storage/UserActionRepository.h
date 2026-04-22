#pragma once

#include "Database.h"
#include "StorageTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace acell
{
namespace storage
{

class UserActionRepository
{
public:
    explicit UserActionRepository(Database& db);

    std::int64_t insert(const UserActionEntry& action);
    std::vector<UserActionEntry> loadRecent(std::int64_t cell_id, int limit);

private:
    Database& db_;
};

} // namespace storage
} // namespace acell