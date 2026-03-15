#include "services_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_method.hpp>

#include "auth_middleware.hpp"
#include "dto.hpp"

namespace profi::handlers {

namespace fj = userver::formats::json;
namespace hs = userver::server::handlers;

namespace {

fj::Value ToJson(const dto::ServiceResponse& s) {
    fj::ValueBuilder b;
    b["id"] = s.id;
    b["title"] = s.title;
    b["description"] = s.description;
    b["price"] = s.price;
    b["provider_id"] = s.provider_id;
    return b.ExtractValue();
}

dto::ServiceResponse ToDto(const Service& s) {
    return {s.id, s.title, s.description, s.price, s.provider_id};
}

}  // namespace

ServicesHandler::ServicesHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<CatalogStorage>()) {}

std::string ServicesHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    using Method = userver::server::http::HttpMethod;

    if (request.GetMethod() == Method::kGet) {
        fj::ValueBuilder result(fj::Type::kArray);

        for (const auto& s : storage_.GetAll()) {
            result.PushBack(ToJson(ToDto(s)));
        }

        request.GetHttpResponse().SetContentType("application/json");
        return fj::ToString(result.ExtractValue());
    }

    const auto& claims = RequireAuth(context);
    const auto body = fj::FromString(request.RequestBody());
    dto::CreateServiceRequest req{
        body["title"].As<std::string>(""),
        body["description"].As<std::string>(""),
        body["price"].As<double>(0.0),
    };

    if (req.title.empty()) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kClientError>(
            hs::ExternalBody{R"({"error":"title is required"})"});
    }

    const auto svc = storage_.Create(req.title, req.description, req.price, claims.user_id);
    request.GetHttpResponse().SetStatus(userver::server::http::HttpStatus::kCreated);
    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(ToJson(ToDto(svc)));
}

ServiceByIdHandler::ServiceByIdHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<CatalogStorage>()) {}

std::string ServiceByIdHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    const auto svc = storage_.FindById(request.GetPathArg("id"));
    if (!svc) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kResourceNotFound>(
            hs::ExternalBody{R"({"error":"Service not found"})"});
    }
    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(ToJson(ToDto(*svc)));
}

}  // namespace profi::handlers
