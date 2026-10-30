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
inline void registerCategoryRoutes(crow::App<CorsMiddleware> &app, Database &database, TokenStore &tokens) {
    CROW_ROUTE(app, "/api/categories").methods(crow::HTTPMethod::Get)
    ([&database, &tokens](const crow::request &request) {
        bool allowed = false;
        auto denied = requireAdmin(request, tokens, allowed);
        if (!allowed) return denied;
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto statement = sql::prepare(database.get(), "SELECT id,name FROM categories ORDER BY name");
            nlohmann::json data = nlohmann::json::array();
            while (sqlite3_step(statement.get()) == SQLITE_ROW)
                data.push_back({{"id", sqlite3_column_int(statement.get(), 0)}, {"name", sql::text(statement.get(), 1)}});
            return response::success(data);
        } catch (const std::exception &) { return response::error(500, "Could not load categories"); }
    });

    CROW_ROUTE(app, "/api/categories").methods(crow::HTTPMethod::Post)
    ([&database, &tokens](const crow::request &request) {
        bool allowed = false;
        auto denied = requireAdmin(request, tokens, allowed);
        if (!allowed) return denied;
        const auto body = nlohmann::json::parse(request.body, nullptr, false);
        if (body.is_discarded() || !body.is_object() || !body.contains("name") || !body["name"].is_string())
            return response::error(400, "name is required as a string");
        const auto name = body["name"].get<std::string>();
        if (name.empty() || name.size() > 120) return response::error(400, "name must be 1 to 120 characters");
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto statement = sql::prepare(database.get(), "INSERT INTO categories(name) VALUES(?)");
            sql::bindText(statement.get(), 1, name);
            if (!sql::done(statement.get()))
                return sql::uniqueViolation(database.get()) ? response::error(409, "Category name already exists") : response::error(500, "Could not create category");
            return response::success(nlohmann::json{{"id", sqlite3_last_insert_rowid(database.get())}, {"name", name}}, 201);
        } catch (const std::exception &) { return response::error(500, "Could not create category"); }
    });

    CROW_ROUTE(app, "/api/categories/<int>").methods(crow::HTTPMethod::Put)
    ([&database, &tokens](const crow::request &request, int id) {
        bool allowed = false;
        auto denied = requireAdmin(request, tokens, allowed);
        if (!allowed) return denied;
        if (id <= 0) return response::error(400, "id must be positive");
        const auto body = nlohmann::json::parse(request.body, nullptr, false);
        if (body.is_discarded() || !body.is_object() || !body.contains("name") || !body["name"].is_string())
            return response::error(400, "name is required as a string");
        const auto name = body["name"].get<std::string>();
        if (name.empty() || name.size() > 120) return response::error(400, "name must be 1 to 120 characters");
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto statement = sql::prepare(database.get(), "UPDATE categories SET name=? WHERE id=?");
            sql::bindText(statement.get(), 1, name); sql::bindInt(statement.get(), 2, id);
            if (!sql::done(statement.get()))
                return sql::uniqueViolation(database.get()) ? response::error(409, "Category name already exists") : response::error(500, "Could not update category");
            if (sqlite3_changes(database.get()) == 0) return response::error(404, "Category not found");
            return response::success(nlohmann::json{{"id", id}, {"name", name}});
        } catch (const std::exception &) { return response::error(500, "Could not update category"); }
    });

    CROW_ROUTE(app, "/api/categories/<int>").methods(crow::HTTPMethod::Delete)
    ([&database, &tokens](const crow::request &request, int id) {
        bool allowed = false;
        auto denied = requireAdmin(request, tokens, allowed);
        if (!allowed) return denied;
        if (id <= 0) return response::error(400, "id must be positive");
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto statement = sql::prepare(database.get(), "DELETE FROM categories WHERE id=?");
            sql::bindInt(statement.get(), 1, id);
            if (!sql::done(statement.get())) return response::error(500, "Could not delete category");
            if (sqlite3_changes(database.get()) == 0) return response::error(404, "Category not found");
            return crow::response(204);
        } catch (const std::exception &) { return response::error(500, "Could not delete category"); }
    });
}
} // namespace inventory::routes
