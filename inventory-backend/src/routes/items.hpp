#pragma once

#include "../database/database.hpp"
#include "../database/sqlite.hpp"
#include "../middleware/authorization.hpp"
#include "../middleware/cors.hpp"
#include "../utils/response.hpp"

#include <crow.h>
#include <nlohmann/json.hpp>
#include <cmath>
#include <mutex>
#include <stdexcept>
#include <string>

namespace inventory::routes {
struct ItemInput {
    std::string sku, name;
    int categoryId = 0, supplierId = 0, quantity = 0, reorderLevel = 0;
    double unitPrice = 0;
};

inline bool parseItem(const std::string &raw, ItemInput &item, bool allowQuantity) {
    const auto body = nlohmann::json::parse(raw, nullptr, false);
    if (body.is_discarded() || !body.is_object() || !body.contains("sku") || !body["sku"].is_string() ||
        !body.contains("name") || !body["name"].is_string() || !body.contains("unit_price") || !body["unit_price"].is_number()) return false;
    item.sku = body["sku"].get<std::string>(); item.name = body["name"].get<std::string>();
    item.unitPrice = body["unit_price"].get<double>();
    if (item.sku.empty() || item.sku.size() > 80 || item.name.empty() || item.name.size() > 200 ||
        !std::isfinite(item.unitPrice) || item.unitPrice < 0) return false;
    auto optionalId = [&body](const char *key, int &value) {
        value = 0;
        if (!body.contains(key) || body[key].is_null()) return true;
        if (!body[key].is_number_integer()) return false;
        try { value = body[key].get<int>(); } catch (const nlohmann::json::exception &) { return false; }
        return value > 0;
    };
    if (!optionalId("category_id", item.categoryId) || !optionalId("supplier_id", item.supplierId)) return false;
    auto optionalCount = [&body](const char *key, int &value, int defaultValue) {
        value = defaultValue;
        if (!body.contains(key)) return true;
        if (!body[key].is_number_integer()) return false;
        try { value = body[key].get<int>(); } catch (const nlohmann::json::exception &) { return false; }
        return value >= 0;
    };
    if (!optionalCount("reorder_level", item.reorderLevel, 0) ||
        !optionalCount("quantity", item.quantity, 0) || (!allowQuantity && item.quantity != 0)) return false;
    return true;
}

inline nlohmann::json itemJson(sqlite3_stmt *statement) {
    nlohmann::json item = {{"id", sqlite3_column_int(statement, 0)}, {"sku", sql::text(statement, 1)},
        {"name", sql::text(statement, 2)}, {"unit_price", sqlite3_column_double(statement, 5)},
        {"quantity", sqlite3_column_int(statement, 6)}, {"reorder_level", sqlite3_column_int(statement, 7)},
        {"created_at", sql::text(statement, 8)}};
    item["category_id"] = sqlite3_column_type(statement, 3) == SQLITE_NULL ? nlohmann::json(nullptr) : nlohmann::json(sqlite3_column_int(statement, 3));
    item["supplier_id"] = sqlite3_column_type(statement, 4) == SQLITE_NULL ? nlohmann::json(nullptr) : nlohmann::json(sqlite3_column_int(statement, 4));
    return item;
}

inline void bindItemFields(sqlite3_stmt *statement, const ItemInput &item, int firstIndex, bool includeQuantity) {
    sql::bindText(statement, firstIndex, item.sku); sql::bindText(statement, firstIndex + 1, item.name);
    sql::bindNullableInt(statement, firstIndex + 2, item.categoryId == 0 ? nullptr : &item.categoryId);
    sql::bindNullableInt(statement, firstIndex + 3, item.supplierId == 0 ? nullptr : &item.supplierId);
    sql::bindDouble(statement, firstIndex + 4, item.unitPrice);
    if (includeQuantity) {
        sql::bindInt(statement, firstIndex + 5, item.quantity);
        sql::bindInt(statement, firstIndex + 6, item.reorderLevel);
    } else sql::bindInt(statement, firstIndex + 5, item.reorderLevel);
}

inline void registerItemRoutes(crow::App<CorsMiddleware> &app, Database &database, TokenStore &tokens) {
    CROW_ROUTE(app, "/api/items").methods(crow::HTTPMethod::Get)
    ([&database, &tokens](const crow::request &request) {
        bool allowed = false; auto denied = requireAuthenticated(request, tokens, allowed); if (!allowed) return denied;
        const char *qValue = request.url_params.get("q");
        const char *categoryValue = request.url_params.get("category_id");
        const char *pageValue = request.url_params.get("page");
        const char *limitValue = request.url_params.get("limit");
        const std::string query = qValue ? qValue : "";
        int category = 0, page = 1, limit = 20;
        try {
            if (categoryValue) category = std::stoi(categoryValue);
            if (pageValue) page = std::stoi(pageValue);
            if (limitValue) limit = std::stoi(limitValue);
        } catch (const std::exception &) { return response::error(400, "page, limit and category_id must be integers"); }
        if (category < 0 || page < 1 || page > 1000000 || limit < 1 || limit > 100) return response::error(400, "Use page from 1 to 1000000, limit from 1 to 100, and category_id >= 0");
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto count = sql::prepare(database.get(), "SELECT COUNT(*) FROM items WHERE (?='' OR name LIKE '%'||?||'%' OR sku LIKE '%'||?||'%') AND (?=0 OR category_id=?)");
            sql::bindText(count.get(), 1, query); sql::bindText(count.get(), 2, query); sql::bindText(count.get(), 3, query);
            sql::bindInt(count.get(), 4, category); sql::bindInt(count.get(), 5, category);
            if (sqlite3_step(count.get()) != SQLITE_ROW) return response::error(500, "Could not count items");
            const int total = sqlite3_column_int(count.get(), 0);
            auto statement = sql::prepare(database.get(), "SELECT id,sku,name,category_id,supplier_id,unit_price,quantity,reorder_level,created_at FROM items WHERE (?='' OR name LIKE '%'||?||'%' OR sku LIKE '%'||?||'%') AND (?=0 OR category_id=?) ORDER BY id LIMIT ? OFFSET ?");
            sql::bindText(statement.get(), 1, query); sql::bindText(statement.get(), 2, query); sql::bindText(statement.get(), 3, query);
            sql::bindInt(statement.get(), 4, category); sql::bindInt(statement.get(), 5, category);
            sql::bindInt(statement.get(), 6, limit); sql::bindInt(statement.get(), 7, (page - 1) * limit);
            nlohmann::json data = nlohmann::json::array();
            while (sqlite3_step(statement.get()) == SQLITE_ROW) data.push_back(itemJson(statement.get()));
            const int pages = total == 0 ? 0 : (total + limit - 1) / limit;
            return response::success(nlohmann::json{{"items", data}, {"page", page}, {"limit", limit}, {"total", total}, {"total_pages", pages}});
        } catch (const std::exception &) { return response::error(500, "Could not load items"); }
    });

    CROW_ROUTE(app, "/api/items/<int>").methods(crow::HTTPMethod::Get)
    ([&database, &tokens](const crow::request &request, int id) {
        bool allowed = false; auto denied = requireAuthenticated(request, tokens, allowed); if (!allowed) return denied;
        if (id <= 0) return response::error(400, "id must be positive");
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto statement = sql::prepare(database.get(), "SELECT id,sku,name,category_id,supplier_id,unit_price,quantity,reorder_level,created_at FROM items WHERE id=?");
            sql::bindInt(statement.get(), 1, id);
            if (sqlite3_step(statement.get()) != SQLITE_ROW) return response::error(404, "Item not found");
            return response::success(itemJson(statement.get()));
        } catch (const std::exception &) { return response::error(500, "Could not load item"); }
    });

    CROW_ROUTE(app, "/api/items").methods(crow::HTTPMethod::Post)
    ([&database, &tokens](const crow::request &request) {
        bool allowed = false; auto denied = requireAdmin(request, tokens, allowed); if (!allowed) return denied;
        ItemInput item;
        if (!parseItem(request.body, item, true)) return response::error(400, "Invalid item fields; provide SKU, name, non-negative unit_price, quantity and reorder_level");
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto statement = sql::prepare(database.get(), "INSERT INTO items(sku,name,category_id,supplier_id,unit_price,quantity,reorder_level) VALUES(?,?,?,?,?,?,?)");
            bindItemFields(statement.get(), item, 1, true);
            if (!sql::done(statement.get())) return sql::uniqueViolation(database.get()) ? response::error(409, "SKU already exists") : response::error(400, "Category or supplier does not exist");
            auto result = nlohmann::json{{"id", sqlite3_last_insert_rowid(database.get())}, {"sku", item.sku}, {"name", item.name},
                {"category_id", item.categoryId ? nlohmann::json(item.categoryId) : nlohmann::json(nullptr)},
                {"supplier_id", item.supplierId ? nlohmann::json(item.supplierId) : nlohmann::json(nullptr)},
                {"unit_price", item.unitPrice}, {"quantity", item.quantity}, {"reorder_level", item.reorderLevel}};
            return response::success(result, 201);
        } catch (const std::exception &) { return response::error(500, "Could not create item"); }
    });

    CROW_ROUTE(app, "/api/items/<int>").methods(crow::HTTPMethod::Put)
    ([&database, &tokens](const crow::request &request, int id) {
        bool allowed = false; auto denied = requireAdmin(request, tokens, allowed); if (!allowed) return denied;
        if (id <= 0) return response::error(400, "id must be positive");
        ItemInput item;
        if (!parseItem(request.body, item, false)) return response::error(400, "Invalid item fields; provide SKU, name and non-negative unit_price");
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto statement = sql::prepare(database.get(), "UPDATE items SET sku=?,name=?,category_id=?,supplier_id=?,unit_price=?,reorder_level=? WHERE id=?");
            bindItemFields(statement.get(), item, 1, false); sql::bindInt(statement.get(), 7, id);
            if (!sql::done(statement.get())) return sql::uniqueViolation(database.get()) ? response::error(409, "SKU already exists") : response::error(400, "Category or supplier does not exist");
            if (sqlite3_changes(database.get()) == 0) return response::error(404, "Item not found");
            return response::success(nlohmann::json{{"id", id}, {"sku", item.sku}, {"name", item.name},
                {"category_id", item.categoryId ? nlohmann::json(item.categoryId) : nlohmann::json(nullptr)},
                {"supplier_id", item.supplierId ? nlohmann::json(item.supplierId) : nlohmann::json(nullptr)},
                {"unit_price", item.unitPrice}, {"reorder_level", item.reorderLevel}});
        } catch (const std::exception &) { return response::error(500, "Could not update item"); }
    });

    CROW_ROUTE(app, "/api/items/<int>").methods(crow::HTTPMethod::Delete)
    ([&database, &tokens](const crow::request &request, int id) {
        bool allowed = false; auto denied = requireAdmin(request, tokens, allowed); if (!allowed) return denied;
        if (id <= 0) return response::error(400, "id must be positive");
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto statement = sql::prepare(database.get(), "DELETE FROM items WHERE id=?"); sql::bindInt(statement.get(), 1, id);
            if (!sql::done(statement.get())) return response::error(409, "Item cannot be deleted");
            if (sqlite3_changes(database.get()) == 0) return response::error(404, "Item not found");
            return crow::response(204);
        } catch (const std::exception &) { return response::error(409, "Item cannot be deleted"); }
    });
}
} // namespace inventory::routes
