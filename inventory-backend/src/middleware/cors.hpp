#pragma once

#include <crow.h>
#include <string>

// Restrict browser access to the two local development origins used by the React app.
struct CorsMiddleware {
    struct context {};

    void before_handle(crow::request &, crow::response &, context &) {}

    void after_handle(crow::request &request, crow::response &response, context &) {
        const auto origin = request.get_header_value("Origin");
        if (origin == "http://localhost:5173" || origin == "http://localhost:3000") {
            response.add_header("Access-Control-Allow-Origin", origin);
            response.add_header("Vary", "Origin");
        }
        response.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
        response.add_header("Access-Control-Allow-Headers", "Authorization, Content-Type");
        response.add_header("Access-Control-Max-Age", "600");
    }
};
