#pragma once

#include <sqlite3.h>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

namespace inventory {

class Database {
  public:
    explicit Database(const std::string &path);
    sqlite3 *get() const noexcept { return handle_.get(); }
    std::mutex &mutex() noexcept { return mutex_; }

  private:
    struct Closer { void operator()(sqlite3 *db) const noexcept { if (db) sqlite3_close(db); } };
    std::unique_ptr<sqlite3, Closer> handle_;
    std::mutex mutex_;
};

} // namespace inventory
