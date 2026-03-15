#include "auth_handler.hpp"

#include <openssl/sha.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>

#include "auth_middleware.hpp"
#include "dto.hpp"

namespace profi::handlers {

namespace fj = userver::formats::json;
namespace hs = userver::server::handlers;

namespace {

std::string Sha256Hex(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);
    char hex[SHA256_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        snprintf(hex + i * 2, 3, "%02x", hash[i]);
    }
    return std::string(hex, SHA256_DIGEST_LENGTH * 2);
}

fj::Value ToJson(const dto::UserResponse& u) {
    fj::ValueBuilder b;
    b["id"] = u.id;
    b["login"] = u.login;
    b["email"] = u.email;
    b["first_name"] = u.first_name;
    b["last_name"] = u.last_name;
    return b.ExtractValue();
}

dto::UserResponse ToDto(const User& u) {
    return {u.id, u.login, u.email, u.first_name, u.last_name};
}

}  // namespace

RegisterHandler::RegisterHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<UserStorage>()) {}

std::string RegisterHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    const auto body = fj::FromString(request.RequestBody());
    dto::RegisterRequest req{
        body["login"].As<std::string>(""),
        body["email"].As<std::string>(""),
        body["first_name"].As<std::string>(""),
        body["last_name"].As<std::string>(""),
        body["password"].As<std::string>(""),
    };

    if (req.login.empty() || req.password.empty() ||
        req.first_name.empty() || req.last_name.empty()) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kClientError>(
            hs::ExternalBody{R"({"error":"login, first_name, last_name, password are required"})"});
    }

    if (storage_.FindByLogin(req.login)) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kConflictState>(
            hs::ExternalBody{R"({"error":"User with this login already exists"})"});
    }

    const auto user = storage_.Create(
        req.login, req.email, req.first_name, req.last_name, Sha256Hex(req.password));
    request.GetHttpResponse().SetStatus(userver::server::http::HttpStatus::kCreated);
    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(ToJson(ToDto(user)));
}

LoginHandler::LoginHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<UserStorage>()) {}

std::string LoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    const auto body = fj::FromString(request.RequestBody());
    dto::LoginRequest req{
        body["login"].As<std::string>(""),
        body["password"].As<std::string>(""),
    };

    if (req.login.empty() || req.password.empty()) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kClientError>(
            hs::ExternalBody{R"({"error":"login and password are required"})"});
    }

    const auto user = storage_.FindByLogin(req.login);

    if (!user || user->password_hash != Sha256Hex(req.password)) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kUnauthorized>(
            hs::ExternalBody{R"({"error":"Invalid login or password"})"});
    }

    fj::ValueBuilder resp;
    resp["token"] = jwt::Create(user->id, user->login, kJwtSecret);
    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(resp.ExtractValue());
}

}  // namespace profi::handlers
