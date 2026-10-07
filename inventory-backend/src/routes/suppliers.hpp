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
#include <string>

namespace inventory::routes {
inline bool supplierPayload(const std::string &raw, std::string &name, std::string &phone,
                            std::string &email, std::string &address) {
    const auto body = nlohmann::json::parse(raw, nullptr, false);
    if (body.is_discarded() || !body.is_object() || !body.contains("name") || !body["name"].is_string()) return false;
    name = body["name"].get<std::string>();
    if (name.empty() || name.size() > 160) return false;
    auto optionalString = [&body](const char *key, std::string &value, std::size_t maximum) {
        if (!body.contains(key)) { value.clear(); return true; }
        if (!body[key].is_string()) return false;
        value = body[key].get<std::string>();
        return value.size() <= maximum;
    };
    return optionalString("phone", phone, 40) && optionalString("email", email, 254) &&
           optionalString("address", address, 1000) && (email.empty() || email.find('@') != std::string::npos);
}

inline void registerSupplierRoutes(crow::App<CorsMiddleware> &app, Database &database, TokenStore &tokens) {
    CROW_ROUTE(app, "/api/suppliers").methods(crow::HTTPMethod::Get)
    ([&database, &tokens](const crow::request &request) {
        bool allowed = false; auto denied = requireAdmin(request, tokens, allowed); if (!allowed) return denied;
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto statement = sql::prepare(database.get(), "SELECT id,name,phone,email,address FROM suppliers ORDER BY name");
            nlohmann::json data = nlohmann::json::array();
            while (sqlite3_step(statement.get()) == SQLITE_ROW)
                data.push_back({{"id", sqlite3_column_int(statement.get(), 0)}, {"name", sql::text(statement.get(), 1)},
                                {"phone", sql::text(statement.get(), 2)}, {"email", sql::text(statement.get(), 3)},
                                {"address", sql::text(statement.get(), 4)}});
            return response::success(data);
        } catch (const std::exception &) { return response::error(500, "Could not load suppliers"); }
    });

    CROW_ROUTE(app, "/api/suppliers").methods(crow::HTTPMethod::Post)
    ([&database, &tokens](const crow::request &request) {
        bool allowed = false; auto denied = requireAdmin(request, tokens, allowed); if (!allowed) return denied;
        std::string name, phone, email, address;
        if (!supplierPayload(request.body, name, phone, email, address)) return response::error(400, "Invalid supplier fields");
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto statement = sql::prepare(database.get(), "INSERT INTO suppliers(name,phone,email,address) VALUES(?,?,?,?)");
            sql::bindText(statement.get(), 1, name); sql::bindText(statement.get(), 2, phone);
            sql::bindText(statement.get(), 3, email); sql::bindText(statement.get(), 4, address);
            if (!sql::done(statement.get())) return response::error(500, "Could not create supplier");
            return response::success(nlohmann::json{{"id", sqlite3_last_insert_rowid(database.get())}, {"name", name},
                {"phone", phone}, {"email", email}, {"address", address}}, 201);
        } catch (const std::exception &) { return response::error(500, "Could not create supplier"); }
    });

    CROW_ROUTE(app, "/api/suppliers/<int>").methods(crow::HTTPMethod::Put)
    ([&database, &tokens](const crow::request &request, int id) {
        bool allowed = false; auto denied = requireAdmin(request, tokens, allowed); if (!allowed) return denied;
        if (id <= 0) return response::error(400, "id must be positive");
        std::string name, phone, email, address;
        if (!supplierPayload(request.body, name, phone, email, address)) return response::error(400, "Invalid supplier fields");
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto statement = sql::prepare(database.get(), "UPDATE suppliers SET name=?,phone=?,email=?,address=? WHERE id=?");
            sql::bindText(statement.get(), 1, name); sql::bindText(statement.get(), 2, phone);
            sql::bindText(statement.get(), 3, email); sql::bindText(statement.get(), 4, address); sql::bindInt(statement.get(), 5, id);
            if (!sql::done(statement.get())) return response::error(500, "Could not update supplier");
            if (sqlite3_changes(database.get()) == 0) return response::error(404, "Supplier not found");
            return response::success(nlohmann::json{{"id", id}, {"name", name}, {"phone", phone}, {"email", email}, {"address", address}});
        } catch (const std::exception &) { return response::error(500, "Could not update supplier"); }
    });

    CROW_ROUTE(app, "/api/suppliers/<int>").methods(crow::HTTPMethod::Delete)
    ([&database, &tokens](const crow::request &request, int id) {
        bool allowed = false; auto denied = requireAdmin(request, tokens, allowed); if (!allowed) return denied;
        if (id <= 0) return response::error(400, "id must be positive");
        try {
            std::lock_guard<std::mutex> lock(database.mutex());
            auto statement = sql::prepare(database.get(), "DELETE FROM suppliers WHERE id=?");
            sql::bindInt(statement.get(), 1, id);
            if (!sql::done(statement.get())) return response::error(409, "Supplier is in use");
            if (sqlite3_changes(database.get()) == 0) return response::error(404, "Supplier not found");
            return crow::response(204);
        } catch (const std::exception &) { return response::error(409, "Supplier is in use"); }
    });
}
} // namespace inventory::routes
