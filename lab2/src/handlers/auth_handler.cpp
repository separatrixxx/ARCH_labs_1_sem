#include "auth_handler.hpp"

#include <openssl/sha.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>

#include "auth.hpp"

namespace profi::handlers {

namespace fj = userver::formats::json;
namespace hs = userver::server::handlers;

namespace {

std::string Sha256Hex(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);
    char hex[SHA256_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        snprintf(hex + i * 2, 3, "%02x", hash[i]);
    return std::string(hex, SHA256_DIGEST_LENGTH * 2);
}

fj::Value UserToJson(const User& u) {
    fj::ValueBuilder b;
    b["id"] = u.id;
    b["login"] = u.login;
    b["first_name"] = u.first_name;
    b["last_name"] = u.last_name;
    return b.ExtractValue();
}

}  // namespace

RegisterHandler::RegisterHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StorageComponent>()) {}

std::string RegisterHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    const auto body = fj::FromString(request.RequestBody());
    const auto login = body["login"].As<std::string>("");
    const auto first_name = body["first_name"].As<std::string>("");
    const auto last_name = body["last_name"].As<std::string>("");
    const auto password = body["password"].As<std::string>("");

    if (login.empty() || password.empty() || first_name.empty() || last_name.empty()) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kClientError>(
            hs::ExternalBody{R"({"error":"login, first_name, last_name, password are required"})"});
    }
    if (storage_.FindUserByLogin(login)) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kConflictState>(
            hs::ExternalBody{R"({"error":"User with this login already exists"})"});
    }

    const auto user = storage_.CreateUser(login, first_name, last_name, Sha256Hex(password));
    request.GetHttpResponse().SetStatus(userver::server::http::HttpStatus::kCreated);
    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(UserToJson(user));
}

LoginHandler::LoginHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StorageComponent>()) {}

std::string LoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    const auto body = fj::FromString(request.RequestBody());
    const auto login = body["login"].As<std::string>("");
    const auto password = body["password"].As<std::string>("");

    if (login.empty() || password.empty()) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kClientError>(
            hs::ExternalBody{R"({"error":"login and password are required"})"});
    }
    const auto user = storage_.FindUserByLogin(login);
    if (!user || user->password_hash != Sha256Hex(password)) {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kUnauthorized>(
            hs::ExternalBody{R"({"error":"Invalid login or password"})"});
    }

    fj::ValueBuilder resp;
    resp["token"] = jwt::Create(user->id, user->login, kJwtSecret);
    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(resp.ExtractValue());
}

}  // namespace profi::handlers
