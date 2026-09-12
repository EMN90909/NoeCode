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
};

} // namespace noe
