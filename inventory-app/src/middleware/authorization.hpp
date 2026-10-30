#pragma once

#include "auth.hpp"
#include "../utils/response.hpp"

namespace inventory {
inline std::optional<Principal> requestPrincipal(const crow::request &request, TokenStore &tokens) {
    return tokens.find(bearerToken(request.get_header_value("Authorization")));
}

inline crow::response requireAdmin(const crow::request &request, TokenStore &tokens, bool &allowed) {
    const auto principal = requestPrincipal(request, tokens);
    if (!principal) {
        allowed = false;
        return response::error(401, "Authentication required");
    }
    if (principal->role != "admin") {
        allowed = false;
        return response::error(403, "Admin role required");
    }
    allowed = true;
    return crow::response(200);
}

inline crow::response requireAuthenticated(const crow::request &request, TokenStore &tokens, bool &allowed) {
    if (!requestPrincipal(request, tokens)) {
        allowed = false;
        return response::error(401, "Authentication required");
    }
    allowed = true;
    return crow::response(200);
}
} // namespace inventory
