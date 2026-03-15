#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "storage.hpp"

namespace profi::handlers {

class OrderServicesHandler final
    : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-order-services";

    OrderServicesHandler(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    StorageComponent& storage_;
};

}  // namespace profi::handlers
