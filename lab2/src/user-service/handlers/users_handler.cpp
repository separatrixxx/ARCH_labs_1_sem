#include "users_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>

#include "auth_middleware.hpp"
#include "dto.hpp"

namespace profi::handlers {

namespace fj = userver::formats::json;
namespace hs = userver::server::handlers;

namespace {

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

}

UsersHandler::UsersHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<UserStorage>()) {}

std::string UsersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    RequireAuth(context);

    const auto login = request.GetArg("login");
    const auto first_name = request.GetArg("first_name");
    const auto last_name = request.GetArg("last_name");

    fj::ValueBuilder result(fj::Type::kArray);

    if (!login.empty()) {
        const auto user = storage_.FindByLogin(login);
        if (user) {
            result.PushBack(ToJson(ToDto(*user)));
        }
    } else if (!first_name.empty() || !last_name.empty()) {
        for (const auto& u : storage_.FindByNameMask(first_name, last_name)) {
            result.PushBack(ToJson(ToDto(u)));
        }
    } else {
        for (const auto& u : storage_.FindAll()) {
            result.PushBack(ToJson(ToDto(u)));
        }
    }

    request.GetHttpResponse().SetContentType("application/json");
    return fj::ToString(result.ExtractValue());
}

}
