#include "services_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_method.hpp>

#include "auth.hpp"

namespace profi::handlers {

namespace fj = userver::formats::json;
namespace hs = userver::server::handlers;

namespace {

fj::Value ServiceToJson(const Service& s) {
    fj::ValueBuilder b;
    b["id"] = s.id;
    b["title"] = s.title;
    b["description"] = s.description;
    b["price"] = s.price;
    b["provider_id"] = s.provider_id;
    return b.ExtractValue();
}

}  // namespace

ServicesHandler::ServicesHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StorageComponent>()) {}

std::string ServicesHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    using Method = userver::server::http::HttpMethod;

    if (request.GetMethod() == Method::kGet) {
        fj::ValueBuilder result(fj::Type::kArray);
        for (const auto& s : storage_.GetAllServices())
            result.PushBack(ServiceToJson(s));
        request.GetHttpResponse().SetContentType("application/json");
        return fj::ToString(result.ExtractValue());
    }

    const auto claims = RequireAuth(request);
    const auto body = fj::FromString(request.RequestBody());
    const auto title = body["title"].As<std::string>("");
    const auto description = body["description"].As<std::string>("");
    const auto price = body["price"].As<double>(0.0);

    if (title.empty()) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kClientError>(
            hs::ExternalBody{R"({"error":"title is required"})"});
    }

    const auto svc = storage_.CreateService(title, description, price, claims.user_id);
    request.GetHttpResponse().SetStatus(userver::server::http::HttpStatus::kCreated);
    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(ServiceToJson(svc));
}

ServiceByIdHandler::ServiceByIdHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StorageComponent>()) {}

std::string ServiceByIdHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    const auto id = request.GetPathArg("id");
    const auto svc = storage_.FindServiceById(id);
    if (!svc) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kResourceNotFound>(
            hs::ExternalBody{R"({"error":"Service not found"})"});
    }
    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(ServiceToJson(*svc));
}

}  // namespace profi::handlers
