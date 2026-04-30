#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "storage.hpp"

#ifdef USE_CACHE
#include "redis_cache.hpp"
#endif

namespace profi::handlers {

class ServicesHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-services";

    ServicesHandler(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    CatalogStorage& storage_;
#ifdef USE_CACHE
    RedisCache& redis_;
#endif
};

class ServiceByIdHandler final
    : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-service-by-id";

    ServiceByIdHandler(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    CatalogStorage& storage_;
#ifdef USE_CACHE
    RedisCache& redis_;
#endif
};

}
