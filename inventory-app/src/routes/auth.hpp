#pragma once

#include "../database/database.hpp"
#include "../middleware/auth.hpp"
#include "../middleware/cors.hpp"
#include "../utils/password.hpp"
#include "../utils/response.hpp"

#include <crow.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <string>

namespace inventory::routes {

inline bool stringField(const nlohmann::json &body, const char *field) {
    return body.contains(field) && body.at(field).is_string();
}

inline void registerAuthRoutes(crow::App<CorsMiddleware> &app, Database &database, TokenStore &tokens) {
    CROW_ROUTE(app, "/api/auth/register").methods(crow::HTTPMethod::Post)
    ([&database, &tokens](const crow::request &request) {
        const auto body = nlohmann::json::parse(request.body, nullptr, false);
        if (body.is_discarded() || !body.is_object() || !stringField(body, "name") ||
            !stringField(body, "email") || !stringField(body, "password"))
            return response::error(400, "name, email and password are required as strings");
        const std::string name = body.at("name").get<std::string>();
        const std::string email = body.at("email").get<std::string>();
        const std::string plain = body.at("password").get<std::string>();
        if (name.empty() || name.size() > 120 || email.empty() || email.size() > 254 || email.find('@') == std::string::npos ||
            plain.size() < 8 || plain.size() > 128)
            return response::error(400, "Use a valid name and email; password must be 8 to 128 characters");

        std::lock_guard<std::mutex> lock(database.mutex());
        sqlite3 *db = database.get();
        sqlite3_stmt *countStatement = nullptr;
        if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users", -1, &countStatement, nullptr) != SQLITE_OK)
            return response::error(500, "Database error");
        const bool hasUsers = sqlite3_step(countStatement) == SQLITE_ROW && sqlite3_column_int(countStatement, 0) > 0;
        sqlite3_finalize(countStatement);

        std::string role = "admin";
        if (hasUsers) {
            const auto principal = tokens.find(bearerToken(request.get_header_value("Authorization")));
            if (!principal) return response::error(401, "Authentication required");
            if (principal->role != "admin") return response::error(403, "Admin role required");
            if (body.contains("role")) {
                if (!stringField(body, "role") || (body.at("role") != "admin" && body.at("role") != "staff"))
                    return response::error(400, "role must be admin or staff");
                role = body.at("role").get<std::string>();
            }
        }

        const auto salt = password::randomSalt();
        const auto encoded = salt + ":" + password::hash(salt, plain);
        sqlite3_stmt *insert = nullptr;
        if (sqlite3_prepare_v2(db, "INSERT INTO users(name,email,password_hash,role) VALUES(?,?,?,?)", -1, &insert, nullptr) != SQLITE_OK)
            return response::error(500, "Database error");
        sqlite3_bind_text(insert, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert, 2, email.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert, 3, encoded.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert, 4, role.c_str(), -1, SQLITE_TRANSIENT);
        const int result = sqlite3_step(insert);
        sqlite3_finalize(insert);
        if (result != SQLITE_DONE) {
            if (sqlite3_extended_errcode(db) == SQLITE_CONSTRAINT_UNIQUE)
                return response::error(409, "Email is already registered");
            return response::error(500, "Could not create user");
        }

        Principal created{static_cast<int>(sqlite3_last_insert_rowid(db)), name, email, role};
        crow::json::wvalue data;
        data["id"] = created.id;
        data["name"] = created.name;
        data["email"] = created.email;
        data["role"] = created.role;
        data["token"] = tokens.issue(created);
        return response::success(std::move(data), 201);
    });

    CROW_ROUTE(app, "/api/auth/login").methods(crow::HTTPMethod::Post)
    ([&database, &tokens](const crow::request &request) {
        const auto body = nlohmann::json::parse(request.body, nullptr, false);
        if (body.is_discarded() || !body.is_object() || !stringField(body, "email") || !stringField(body, "password"))
            return response::error(400, "email and password are required as strings");
        const std::string email = body.at("email").get<std::string>();
        const std::string plain = body.at("password").get<std::string>();

        std::lock_guard<std::mutex> lock(database.mutex());
        sqlite3_stmt *statement = nullptr;
        if (sqlite3_prepare_v2(database.get(), "SELECT id,name,email,password_hash,role FROM users WHERE email=?", -1, &statement, nullptr) != SQLITE_OK)
            return response::error(500, "Database error");
        sqlite3_bind_text(statement, 1, email.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(statement) != SQLITE_ROW) {
            sqlite3_finalize(statement);
            return response::error(401, "Invalid email or password");
        }
        Principal principal{sqlite3_column_int(statement, 0),
                            reinterpret_cast<const char *>(sqlite3_column_text(statement, 1)),
                            reinterpret_cast<const char *>(sqlite3_column_text(statement, 2)),
                            reinterpret_cast<const char *>(sqlite3_column_text(statement, 4))};
        const std::string encoded = reinterpret_cast<const char *>(sqlite3_column_text(statement, 3));
        sqlite3_finalize(statement);
        const auto separator = encoded.find(':');
        if (separator == std::string::npos || password::hash(encoded.substr(0, separator), plain) != encoded.substr(separator + 1))
            return response::error(401, "Invalid email or password");

        crow::json::wvalue data;
        data["id"] = principal.id;
        data["name"] = principal.name;
        data["email"] = principal.email;
        data["role"] = principal.role;
        data["token"] = tokens.issue(principal);
        return response::success(std::move(data));
    });

    CROW_ROUTE(app, "/api/auth/me").methods(crow::HTTPMethod::Get)
    ([&tokens](const crow::request &request) {
        const auto principal = tokens.find(bearerToken(request.get_header_value("Authorization")));
        if (!principal) return response::error(401, "Authentication required");
        crow::json::wvalue data;
        data["id"] = principal->id;
        data["name"] = principal->name;
        data["email"] = principal->email;
        data["role"] = principal->role;
        return response::success(std::move(data));
    });
}
} // namespace inventory::routes
