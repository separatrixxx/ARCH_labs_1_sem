#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "storage.hpp"

namespace profi::handlers {

class RegisterHandler final
    : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-auth-register";

    RegisterHandler(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    UserStorage& storage_;
};

class LoginHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-auth-login";

    LoginHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    UserStorage& storage_;
};

}  // namespace profi::handlers
