#pragma once

#include "../database/database.hpp"
#include "../database/sqlite.hpp"
#include "../middleware/authorization.hpp"
#include "../middleware/cors.hpp"
#include "../utils/response.hpp"

#include <crow.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <stdexcept>

namespace inventory::routes {
inline void registerDashboardRoutes(crow::App<CorsMiddleware> &app, Database &database, TokenStore &tokens) {
    CROW_ROUTE(app, "/api/dashboard").methods(crow::HTTPMethod::Get)
    ([&database, &tokens](const crow::request &request) {
        bool allowed = false; auto denied = requireAuthenticated(request, tokens, allowed); if (!allowed) return denied;
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto totals = sql::prepare(database.get(),
                "SELECT COUNT(*),COALESCE(SUM(quantity*unit_price),0),SUM(CASE WHEN quantity<=reorder_level THEN 1 ELSE 0 END) FROM items");
            if (sqlite3_step(totals.get()) != SQLITE_ROW) return response::error(500, "Could not load dashboard totals");
            nlohmann::json data = {{"total_items", sqlite3_column_int(totals.get(), 0)},
                {"total_stock_value", sqlite3_column_double(totals.get(), 1)},
                {"low_stock_count", sqlite3_column_int(totals.get(), 2)}};

            auto recent = sql::prepare(database.get(),
                "SELECT m.id,m.item_id,i.sku,m.type,m.quantity,m.note,m.user_id,u.name,m.created_at "
                "FROM stock_movements m JOIN items i ON i.id=m.item_id LEFT JOIN users u ON u.id=m.user_id "
                "ORDER BY m.id DESC LIMIT ?");
            sql::bindInt(recent.get(), 1, 5);
            data["recent_movements"] = nlohmann::json::array();
            while (sqlite3_step(recent.get()) == SQLITE_ROW) {
                nlohmann::json row = {{"id", sqlite3_column_int(recent.get(), 0)},
                    {"item_id", sqlite3_column_int(recent.get(), 1)}, {"sku", sql::text(recent.get(), 2)},
                    {"type", sql::text(recent.get(), 3)}, {"quantity", sqlite3_column_int(recent.get(), 4)},
                    {"note", sql::text(recent.get(), 5)}, {"created_at", sql::text(recent.get(), 8)}};
                row["user_id"] = sqlite3_column_type(recent.get(), 6) == SQLITE_NULL
                    ? nlohmann::json(nullptr) : nlohmann::json(sqlite3_column_int(recent.get(), 6));
                row["user_name"] = sqlite3_column_type(recent.get(), 7) == SQLITE_NULL
                    ? nlohmann::json(nullptr) : nlohmann::json(sql::text(recent.get(), 7));
                data["recent_movements"].push_back(std::move(row));
            }
            return response::success(data);
        } catch (const std::exception &) { return response::error(500, "Could not load dashboard"); }
    });
}
} // namespace inventory::routes
