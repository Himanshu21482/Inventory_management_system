#pragma once

#include <crow.h>
#include <algorithm>
#include <cctype>
#include <string>

// Allow the two localhost development origins used by the frontend.
struct CorsMiddleware {
    struct context {};

    void before_handle(crow::request &request, crow::response &response, context &) {
        if (request.method == crow::HTTPMethod::Options) addHeaders(request, response);
    }

    void after_handle(crow::request &request, crow::response &response, context &) {
        addHeaders(request, response);
    }

  private:
    static void addHeaders(crow::request &request, crow::response &response) {
        std::string origin = request.get_header_value("origin");
        const auto first = origin.find_first_not_of(" \t\r\n");
        const auto last = origin.find_last_not_of(" \t\r\n");
        origin = first == std::string::npos ? std::string{} : origin.substr(first, last - first + 1);
        std::string normalized = origin;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                       [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
        if (normalized == "http://localhost:5173" || normalized == "http://localhost:3000") {
            response.set_header("Access-Control-Allow-Origin", origin);
            response.set_header("Vary", "Origin");
        }
        response.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
        response.set_header("Access-Control-Allow-Headers", "Authorization, Content-Type");
        response.set_header("Access-Control-Max-Age", "600");
    }
};
