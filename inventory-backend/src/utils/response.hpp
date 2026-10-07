#pragma once

#include <crow.h>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

namespace inventory::response {
inline crow::response success(const nlohmann::json &data, int status = 200) {
    crow::response result(status);
    result.set_header("Content-Type", "application/json; charset=utf-8");
    result.body = nlohmann::json{{"success", true}, {"data", data}}.dump();
    return result;
}

inline crow::response success(crow::json::wvalue data, int status = 200) {
    crow::json::wvalue body;
    body["success"] = true;
    body["data"] = std::move(data);
    crow::response result(status);
    result.set_header("Content-Type", "application/json; charset=utf-8");
    result.body = body.dump();
    return result;
}

inline crow::response error(int status, const std::string &message) {
    crow::json::wvalue body;
    body["success"] = false;
    body["message"] = message;
    crow::response result(status);
    result.set_header("Content-Type", "application/json; charset=utf-8");
    result.body = body.dump();
    return result;
}
} // namespace inventory::response
