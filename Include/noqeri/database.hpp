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

    // SQL is an adapter over the same .nqdb storage engine. The supported
    // application subset is CREATE TABLE, INSERT, SELECT, UPDATE, DELETE and
    // BEGIN/COMMIT/ROLLBACK boundaries, with equality WHERE clauses and simple
    // ORDER BY/projection. Unsupported SQL fails closed with a diagnostic.
    bool executeSql(const std::filesystem::path& script,
                    const std::filesystem::path& databaseOverride,
                    std::ostream& output,
                    Diagnostics& diagnostics) const;
};

} // namespace noe
