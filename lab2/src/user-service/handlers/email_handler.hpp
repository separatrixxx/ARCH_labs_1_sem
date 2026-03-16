#pragma once

#include <string>

#include <userver/clients/http/component.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

namespace profi::handlers {

class EmailHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-email";

    static userver::yaml_config::Schema GetStaticConfigSchema();

    EmailHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    userver::clients::http::Client& http_client_;
    std::string api_key_;
    std::string sender_email_;
    std::string sender_name_;
    std::string list_id_;
};

}
