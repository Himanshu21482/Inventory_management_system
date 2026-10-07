#include "database/database.hpp"
#include "middleware/auth.hpp"
#include "middleware/cors.hpp"
#include "routes/auth.hpp"
#include "routes/categories.hpp"
#include "routes/items.hpp"
#include "routes/suppliers.hpp"

#include <crow.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
std::string readFile(const std::string &path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot read " + path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void initializeSchema(sqlite3 *db) {
    const auto sql = readFile("schema.sql");
    char *error = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
        const std::string message = error ? error : sqlite3_errmsg(db);
        sqlite3_free(error);
        throw std::runtime_error("Schema initialization failed: " + message);
    }
}
}

int main() {
    try {
        inventory::Database database("inventory.db");
        initializeSchema(database.get());
        inventory::TokenStore tokens;
        crow::App<CorsMiddleware> app;
        CROW_ROUTE(app, "/api/health").methods(crow::HTTPMethod::Get)([] {
            crow::json::wvalue data;
            data["status"] = "ok";
            return inventory::response::success(std::move(data));
        });
        CROW_ROUTE(app, "/api/<path>").methods(crow::HTTPMethod::Options)([] {
            return crow::response(204);
        });
        inventory::routes::registerAuthRoutes(app, database, tokens);
        inventory::routes::registerCategoryRoutes(app, database, tokens);
        inventory::routes::registerSupplierRoutes(app, database, tokens);
        inventory::routes::registerItemRoutes(app, database, tokens);
        std::cout << "Inventory backend listening on http://localhost:8080\n";
        app.port(8080).multithreaded().run();
    } catch (const std::exception &error) {
        std::cerr << "Startup failed: " << error.what() << '\n';
        return 1;
    }
}
