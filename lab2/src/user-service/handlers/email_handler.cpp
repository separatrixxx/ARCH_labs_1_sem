#include "email_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>

#include "auth_middleware.hpp"

namespace profi::handlers {

namespace fj = userver::formats::json;
namespace hs = userver::server::handlers;

userver::yaml_config::Schema EmailHandler::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<
        userver::server::handlers::HttpHandlerBase>(R"(
type: object
description: email notification handler via Unisender
additionalProperties: false
properties:
    api_key:
        type: string
        description: Unisender API key
    sender_email:
        type: string
        description: verified sender email address
    sender_name:
        type: string
        description: sender display name
    list_id:
        type: string
        description: Unisender mailing list ID
)");
}

EmailHandler::EmailHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      http_client_(context.FindComponent<userver::components::HttpClient>()
                       .GetHttpClient()),
      api_key_(config["api_key"].As<std::string>()),
      sender_email_(config["sender_email"].As<std::string>()),
      sender_name_(config["sender_name"].As<std::string>("Profi Service")),
      list_id_(config["list_id"].As<std::string>("1")) {}

std::string EmailHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    RequireAuth(context);

    const auto body = fj::FromString(request.RequestBody());
    const auto to = body["to"].As<std::string>("");
    const auto subject = body["subject"].As<std::string>("");
    const auto message = body["message"].As<std::string>("");

    if (to.empty() || subject.empty() || message.empty()) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kClientError>(
            hs::ExternalBody{R"({"error":"to, subject, message are required"})"});
    }

    const std::string form =
        "format=json"
        "&api_key=" + api_key_ +
        "&email=" + to +
        "&sender_name=" + sender_name_ +
        "&sender_email=" + sender_email_ +
        "&subject=" + subject +
        "&body=" + message +
        "&list_id=" + list_id_;

    auto resp = http_client_.CreateRequest()
                    .post("https://api.unisender.com/ru/api/sendEmail")
                    .data(form)
                    .headers({{"Content-Type", "application/x-www-form-urlencoded"}})
                    .timeout(std::chrono::seconds{10})
                    .perform();

    request.GetHttpResponse().SetContentType("application/json");
    return resp->body();
}

}  // namespace profi::handlers
