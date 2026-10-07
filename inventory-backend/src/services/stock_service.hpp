#pragma once

#include "../database/database.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace inventory {
struct StockResult {
    bool success = false;
    std::string code;
    int quantity = 0;
};

class StockService {
  public:
    explicit StockService(Database &database) : database_(database) {}
    StockResult move(int itemId, int quantity, const std::string &type,
                     const std::string &note, int userId);
    nlohmann::json movements(std::optional<int> itemId);
    nlohmann::json lowStockItems();

  private:
    Database &database_;
};
} // namespace inventory
