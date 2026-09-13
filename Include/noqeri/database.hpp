#pragma once
#include "diagnostics.hpp"
#include <filesystem>
#include <iosfwd>

namespace noe {

// NoqeriDB executes the compact .nqd data language against a durable .nqdb
// file. A whole script is a transaction: the database file is replaced only
// after every statement parses and validates successfully.
class NoqeriDatabase {
public:
    bool execute(const std::filesystem::path& script,
                 const std::filesystem::path& databaseOverride,
                 std::ostream& output,
                 Diagnostics& diagnostics) const;

    // SQL is an adapter over the same .nqdb storage engine. The bootstrap
    // application surface includes CREATE TABLE, INSERT, SELECT, UPDATE,
    // DELETE and BEGIN/COMMIT/ROLLBACK boundaries; equality WHERE predicates;
    // qualified inner equality JOINs; projection; GROUP BY; COUNT/SUM/MIN/MAX;
    // and ORDER BY selected columns or aliases. The adapter is deliberately
    // fail-closed for unsupported SQL instead of silently changing semantics.
    // More advanced DDL and planner/index optimisation belong to the database
    // engine maturity gates rather than being approximated in this adapter.
    bool executeSql(const std::filesystem::path& script,
                    const std::filesystem::path& databaseOverride,
                    std::ostream& output,
                    Diagnostics& diagnostics) const;
};

} // namespace noe
