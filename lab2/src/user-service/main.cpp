#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include "storage.hpp"
#include "handlers/auth_handler.hpp"
#include "handlers/users_handler.hpp"
#include "handlers/swagger_handler.hpp"
#include "handlers/email_handler.hpp"
#include "auth_middleware.hpp"

int main(int argc, char* argv[]) {
    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<profi::UserStorage>()
            .Append<profi::JwtAuthMiddlewareFactory>()
            .Append<userver::clients::dns::Component>()
            .Append<userver::components::HttpClient>()
            .Append<profi::handlers::RegisterHandler>()
            .Append<profi::handlers::LoginHandler>()
            .Append<profi::handlers::UsersHandler>()
            .Append<profi::handlers::SwaggerUiHandler>()
            .Append<profi::handlers::OpenApiHandler>()
            .Append<profi::handlers::EmailHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
