#pragma once

#include "../middleware/auth.hpp"
#include "../middleware/authorization.hpp"
#include "../middleware/cors.hpp"
#include "../services/stock_service.hpp"
#include "../utils/response.hpp"

#include <crow.h>
#include <nlohmann/json.hpp>
#include <limits>
#include <optional>
#include <string>

namespace inventory::routes {
inline crow::response recordMovement(const crow::request &request, StockService &service,
                                     TokenStore &tokens, const std::string &type);

inline void registerStockRoutes(crow::App<CorsMiddleware> &app, StockService &service, TokenStore &tokens) {
    CROW_ROUTE(app, "/api/stock/in").methods(crow::HTTPMethod::Post)
    ([&service, &tokens](const crow::request &request) {
        return recordMovement(request, service, tokens, "IN");
    });
    CROW_ROUTE(app, "/api/stock/out").methods(crow::HTTPMethod::Post)
    ([&service, &tokens](const crow::request &request) {
        return recordMovement(request, service, tokens, "OUT");
    });

    CROW_ROUTE(app, "/api/stock/movements").methods(crow::HTTPMethod::Get)
    ([&service, &tokens](const crow::request &request) {
        bool allowed = false; auto denied = requireAuthenticated(request, tokens, allowed); if (!allowed) return denied;
        std::optional<int> itemId;
        if (const auto *value = request.url_params.get("item_id")) {
            try { itemId = std::stoi(value); }
            catch (const std::exception &) { return response::error(400, "item_id must be an integer"); }
            if (*itemId <= 0) return response::error(400, "item_id must be positive");
        }
        try { return response::success(service.movements(itemId)); }
        catch (const std::exception &) { return response::error(500, "Could not load stock history"); }
    });

    CROW_ROUTE(app, "/api/items/low-stock").methods(crow::HTTPMethod::Get)
    ([&service, &tokens](const crow::request &request) {
        bool allowed = false; auto denied = requireAuthenticated(request, tokens, allowed); if (!allowed) return denied;
        try { return response::success(service.lowStockItems()); }
        catch (const std::exception &) { return response::error(500, "Could not load low-stock items"); }
    });
}

inline crow::response recordMovement(const crow::request &request, StockService &service,
                                     TokenStore &tokens, const std::string &type) {
    const auto principal = requestPrincipal(request, tokens);
    if (!principal) return response::error(401, "Authentication required");
    const auto body = nlohmann::json::parse(request.body, nullptr, false);
    if (body.is_discarded() || !body.is_object() || !body.contains("item_id") ||
        !body.contains("quantity") || !body["item_id"].is_number_integer() || !body["quantity"].is_number_integer())
        return response::error(400, "item_id and positive integer quantity are required");
    int itemId = 0, quantity = 0;
    try { itemId = body["item_id"].get<int>(); quantity = body["quantity"].get<int>(); }
    catch (const nlohmann::json::exception &) { return response::error(400, "item_id and quantity are out of range"); }
    std::string note;
    if (body.contains("note")) {
        if (!body["note"].is_string()) return response::error(400, "note must be a string");
        note = body["note"].get<std::string>();
    }
    if (note.size() > 500) return response::error(400, "note must be 500 characters or fewer");
    const auto result = service.move(itemId, quantity, type, note, principal->id);
    if (result.code == "INVALID_INPUT") return response::error(400, "item_id and quantity must be positive");
    if (result.code == "ITEM_NOT_FOUND") return response::error(404, "Item not found");
    if (result.code == "INSUFFICIENT_STOCK") return response::error(409, "Not enough stock to complete this stock out");
    if (result.code == "QUANTITY_LIMIT") return response::error(409, "Stock quantity limit would be exceeded");
    if (!result.success) return response::error(500, "Could not record stock movement");
    return response::success(nlohmann::json{{"item_id", itemId}, {"type", type}, {"quantity", quantity},
        {"new_quantity", result.quantity}}, 201);
}
} // namespace inventory::routes
