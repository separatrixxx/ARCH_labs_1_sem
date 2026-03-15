#include "order_services_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>

#include "auth.hpp"

namespace profi::handlers {

namespace fj = userver::formats::json;
namespace hs = userver::server::handlers;

namespace {

fj::Value OrderToJson(const Order& o) {
    fj::ValueBuilder b;
    b["id"] = o.id;
    b["client_id"] = o.client_id;
    b["status"] = o.status;
    fj::ValueBuilder services(fj::Type::kArray);
    for (const auto& sid : o.service_ids) services.PushBack(sid);
    b["service_ids"] = services.ExtractValue();
    return b.ExtractValue();
}

}  // namespace

OrderServicesHandler::OrderServicesHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StorageComponent>()) {}

std::string OrderServicesHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    const auto claims = RequireAuth(request);
    const auto order_id = request.GetPathArg("order_id");

    const auto existing_order = storage_.FindOrderById(order_id);
    if (!existing_order) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kResourceNotFound>(
            hs::ExternalBody{R"({"error":"Order not found"})"});
    }
    if (existing_order->client_id != claims.user_id) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kForbidden>(
            hs::ExternalBody{R"({"error":"You don't own this order"})"});
    }

    const auto body = fj::FromString(request.RequestBody());
    const auto service_id = body["service_id"].As<std::string>("");
    if (service_id.empty()) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kClientError>(
            hs::ExternalBody{R"({"error":"service_id is required"})"});
    }
    if (!storage_.FindServiceById(service_id)) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kResourceNotFound>(
            hs::ExternalBody{R"({"error":"Service not found"})"});
    }

    const auto updated = storage_.AddServiceToOrder(order_id, service_id);
    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(OrderToJson(*updated));
}

}  // namespace profi::handlers
