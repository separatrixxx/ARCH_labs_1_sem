#include "orders_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_method.hpp>

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

OrdersHandler::OrdersHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StorageComponent>()) {}

std::string OrdersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    const auto claims = RequireAuth(request);
    using Method = userver::server::http::HttpMethod;

    if (request.GetMethod() == Method::kGet) {
        fj::ValueBuilder result(fj::Type::kArray);
        for (const auto& o : storage_.GetOrdersByClientId(claims.user_id))
            result.PushBack(OrderToJson(o));
        request.GetHttpResponse().SetContentType("application/json");
        return fj::ToString(result.ExtractValue());
    }

    const auto order = storage_.CreateOrder(claims.user_id);
    request.GetHttpResponse().SetStatus(userver::server::http::HttpStatus::kCreated);
    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(OrderToJson(order));
}

OrderByIdHandler::OrderByIdHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StorageComponent>()) {}

std::string OrderByIdHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    const auto claims = RequireAuth(request);
    const auto id = request.GetPathArg("id");
    const auto order = storage_.FindOrderById(id);
    if (!order) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kResourceNotFound>(
            hs::ExternalBody{R"({"error":"Order not found"})"});
    }
    if (order->client_id != claims.user_id) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kForbidden>(
            hs::ExternalBody{R"({"error":"You don't own this order"})"});
    }
    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(OrderToJson(*order));
}

}  // namespace profi::handlers
