#pragma once

#include "jwt.hpp"

#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/server/request/request_context.hpp>

namespace profi {

inline const std::string kJwtSecret = "profi-secret-key-change-in-prod";
static constexpr std::string_view kJwtClaimsKey = "jwt_claims";

class JwtAuthMiddleware final
    : public userver::server::middlewares::HttpMiddlewareBase {
public:
    static constexpr std::string_view kName{"jwt-auth-middleware"};

    explicit JwtAuthMiddleware(
        const userver::server::handlers::HttpHandlerBase&) {}

private:
    void HandleRequest(userver::server::http::HttpRequest& request,
                       userver::server::request::RequestContext& context) const override {
        const auto& auth = request.GetHeader("Authorization");
        if (auth.size() > 7 && auth.substr(0, 7) == "Bearer ") {
            try {
                auto claims = jwt::Validate(auth.substr(7), kJwtSecret);
                context.SetData(std::string{kJwtClaimsKey}, std::move(claims));
            } catch (...) {}
        }
        Next(request, context);
    }
};

using JwtAuthMiddlewareFactory =
    userver::server::middlewares::SimpleHttpMiddlewareFactory<JwtAuthMiddleware>;

inline const jwt::Claims& RequireAuth(
    userver::server::request::RequestContext& context) {
    const auto* claims =
        context.GetDataOptional<jwt::Claims>(kJwtClaimsKey);
    if (!claims) {
        throw userver::server::handlers::ExceptionWithCode<
            userver::server::handlers::HandlerErrorCode::kUnauthorized>(
            userver::server::handlers::InternalMessage{
                "Authentication required"});
    }
    return *claims;
}

}
