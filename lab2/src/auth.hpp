#pragma once

#include "jwt.hpp"

#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_request.hpp>

namespace profi {

static const std::string kJwtSecret = "profi-secret-key-change-in-prod";

inline jwt::Claims RequireAuth(
    const userver::server::http::HttpRequest& request) {
    const auto& auth = request.GetHeader("Authorization");
    if (auth.size() < 8 || auth.substr(0, 7) != "Bearer ") {
        throw userver::server::handlers::ExceptionWithCode<
            userver::server::handlers::HandlerErrorCode::kUnauthorized>(
            userver::server::handlers::InternalMessage{
                "Missing or invalid Authorization header"});
    }
    try {
        return jwt::Validate(auth.substr(7), kJwtSecret);
    } catch (const std::exception& e) {
        throw userver::server::handlers::ExceptionWithCode<
            userver::server::handlers::HandlerErrorCode::kUnauthorized>(
            userver::server::handlers::InternalMessage{e.what()});
    }
}

}  // namespace profi
