#include "database.hpp"

namespace inventory {

Database::Database(const std::string &path) {
    sqlite3 *raw = nullptr;
    if (sqlite3_open(path.c_str(), &raw) != SQLITE_OK) {
        const std::string message = raw ? sqlite3_errmsg(raw) : "could not allocate SQLite connection";
        if (raw) sqlite3_close(raw);
        throw std::runtime_error("SQLite open failed: " + message);
    }
    handle_.reset(raw);
    sqlite3_busy_timeout(handle_.get(), 5000);
    char *error = nullptr;
    const int result = sqlite3_exec(handle_.get(), "PRAGMA foreign_keys = ON", nullptr, nullptr, &error);
    if (result != SQLITE_OK) {
        const std::string message = error ? error : sqlite3_errmsg(handle_.get());
        sqlite3_free(error);
        throw std::runtime_error("SQLite setup failed: " + message);
    }
}

} // namespace inventory
