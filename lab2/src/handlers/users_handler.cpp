#include "users_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>

#include "auth.hpp"

namespace profi::handlers {

namespace fj = userver::formats::json;
namespace hs = userver::server::handlers;

namespace {

fj::Value UserToJson(const User& u) {
    fj::ValueBuilder b;
    b["id"] = u.id;
    b["login"] = u.login;
    b["first_name"] = u.first_name;
    b["last_name"] = u.last_name;
    return b.ExtractValue();
}

}  // namespace

UsersHandler::UsersHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StorageComponent>()) {}

std::string UsersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    RequireAuth(request);

    const auto login = request.GetArg("login");
    const auto first_name = request.GetArg("first_name");
    const auto last_name = request.GetArg("last_name");

    fj::ValueBuilder result(fj::Type::kArray);

    if (!login.empty()) {
        const auto user = storage_.FindUserByLogin(login);
        if (user) result.PushBack(UserToJson(*user));
    } else if (!first_name.empty() || !last_name.empty()) {
        for (const auto& u : storage_.FindUsersByNameMask(first_name, last_name))
            result.PushBack(UserToJson(u));
    } else {
        throw hs::ExceptionWithCode<hs::HandlerErrorCode::kClientError>(
            hs::ExternalBody{
                R"({"error":"Provide 'login' or 'first_name'/'last_name' query params"})"});
    }

    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(result.ExtractValue());
}

}  // namespace profi::handlers
