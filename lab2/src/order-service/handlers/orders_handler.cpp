#include "orders_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

#include "auth_middleware.hpp"
#include "dto.hpp"

namespace profi::handlers {

namespace fj = userver::formats::json;
namespace hs = userver::server::handlers;

namespace {

fj::Value ToJson(const dto::OrderResponse& o) {
    fj::ValueBuilder b;
    b["id"] = o.id;
    b["client_id"] = o.client_id;
    b["status"] = o.status;
    fj::ValueBuilder sids(fj::Type::kArray);
    for (const auto& sid : o.service_ids) sids.PushBack(sid);
    b["service_ids"] = sids.ExtractValue();
    return b.ExtractValue();
}

dto::OrderResponse ToDto(const Order& o) {
    return {o.id, o.client_id, o.status, o.service_ids};
}

}  // namespace

OrdersHandler::OrdersHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<OrderStorage>()) {}

std::string OrdersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    const auto& claims = RequireAuth(context);
    using Method = userver::server::http::HttpMethod;

    if (request.GetMethod() == Method::kGet) {
        fj::ValueBuilder result(fj::Type::kArray);

        for (const auto& o : storage_.FindByClientId(claims.user_id)) {
            result.PushBack(ToJson(ToDto(o)));
        }

        request.GetHttpResponse().SetContentType("application/json");
        return fj::ToString(result.ExtractValue());
    }

    const auto order = storage_.Create(claims.user_id);
    request.GetHttpResponse().SetStatus(userver::server::http::HttpStatus::kCreated);
    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(ToJson(ToDto(order)));
}

OrderByIdHandler::OrderByIdHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<OrderStorage>()) {}

std::string OrderByIdHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    const auto& claims = RequireAuth(context);
    const auto order = storage_.FindById(request.GetPathArg("id"));

    if (!order) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kResourceNotFound>(
            hs::ExternalBody{R"({"error":"Order not found"})"});
    }

    if (order->client_id != claims.user_id) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kForbidden>(
            hs::ExternalBody{R"({"error":"You don't own this order"})"});
    }

    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(ToJson(ToDto(*order)));
}

userver::yaml_config::Schema OrderServicesHandler::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::server::handlers::HttpHandlerBase>(R"(
type: object
description: handler for adding services to orders
additionalProperties: false
properties:
    catalog_url:
        type: string
        description: base URL of catalog-service
)");
}

OrderServicesHandler::OrderServicesHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<OrderStorage>()),
      http_client_(context.FindComponent<userver::components::HttpClient>()
                       .GetHttpClient()),
      catalog_url_(config["catalog_url"].As<std::string>()) {}

std::string OrderServicesHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    const auto& claims = RequireAuth(context);
    const auto order_id = request.GetPathArg("order_id");

    const auto order = storage_.FindById(order_id);

    if (!order) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kResourceNotFound>(
            hs::ExternalBody{R"({"error":"Order not found"})"});
    }

    if (order->client_id != claims.user_id) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kForbidden>(
            hs::ExternalBody{R"({"error":"You don't own this order"})"});
    }

    const auto body = fj::FromString(request.RequestBody());
    const auto service_id = body["service_id"].As<std::string>("");

    if (service_id.empty()) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kClientError>(
            hs::ExternalBody{R"({"error":"service_id is required"})"});
    }

    auto resp = http_client_.CreateRequest()
                    .get(catalog_url_ + "/api/v1/services/" + service_id)
                    .timeout(std::chrono::seconds{5})
                    .perform();

    if (resp->status_code() == 404) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kResourceNotFound>(
            hs::ExternalBody{R"({"error":"Service not found"})"});
    }

    const auto updated = storage_.AddService(order_id, service_id);

    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(ToJson(ToDto(*updated)));
}

}  // namespace profi::handlers
