#pragma once

#include <sqlite3.h>
#include <memory>
#include <stdexcept>
#include <string>

namespace inventory::sql {
struct StatementDeleter {
    void operator()(sqlite3_stmt *statement) const noexcept { sqlite3_finalize(statement); }
};
using Statement = std::unique_ptr<sqlite3_stmt, StatementDeleter>;

inline Statement prepare(sqlite3 *db, const char *query) {
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(db, query, -1, &statement, nullptr) != SQLITE_OK)
        throw std::runtime_error("SQLite query could not be prepared");
    return Statement(statement);
}
inline void bindText(sqlite3_stmt *statement, int index, const std::string &value) {
    if (sqlite3_bind_text(statement, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
        throw std::runtime_error("SQLite parameter binding failed");
}
inline void bindInt(sqlite3_stmt *statement, int index, int value) {
    if (sqlite3_bind_int(statement, index, value) != SQLITE_OK)
        throw std::runtime_error("SQLite parameter binding failed");
}
inline void bindDouble(sqlite3_stmt *statement, int index, double value) {
    if (sqlite3_bind_double(statement, index, value) != SQLITE_OK)
        throw std::runtime_error("SQLite parameter binding failed");
}
inline void bindNullableInt(sqlite3_stmt *statement, int index, const int *value) {
    if (value == nullptr) {
        if (sqlite3_bind_null(statement, index) != SQLITE_OK) throw std::runtime_error("SQLite parameter binding failed");
    } else bindInt(statement, index, *value);
}
inline std::string text(sqlite3_stmt *statement, int column) {
    const auto *value = sqlite3_column_text(statement, column);
    return value == nullptr ? std::string{} : reinterpret_cast<const char *>(value);
}
inline bool done(sqlite3_stmt *statement) { return sqlite3_step(statement) == SQLITE_DONE; }
inline bool uniqueViolation(sqlite3 *db) { return sqlite3_extended_errcode(db) == SQLITE_CONSTRAINT_UNIQUE; }

class Transaction {
  public:
    explicit Transaction(sqlite3 *db) : db_(db) { execute("BEGIN IMMEDIATE"); }
    Transaction(const Transaction &) = delete;
    Transaction &operator=(const Transaction &) = delete;
    ~Transaction() { if (active_) sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr); }
    void commit() {
        execute("COMMIT");
        active_ = false;
    }

  private:
    void execute(const char *query) {
        char *error = nullptr;
        if (sqlite3_exec(db_, query, nullptr, nullptr, &error) != SQLITE_OK) {
            const std::string message = error ? error : "SQLite transaction failed";
            sqlite3_free(error);
            throw std::runtime_error(message);
        }
    }
    sqlite3 *db_;
    bool active_ = true;
};
} // namespace inventory::sql
