#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include "storage.hpp"
#include "handlers/auth_handler.hpp"
#include "handlers/users_handler.hpp"
#include "handlers/services_handler.hpp"
#include "handlers/orders_handler.hpp"
#include "handlers/order_services_handler.hpp"
#include "handlers/swagger_handler.hpp"

int main(int argc, char* argv[]) {
    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<profi::StorageComponent>()
            .Append<profi::handlers::RegisterHandler>()
            .Append<profi::handlers::LoginHandler>()
            .Append<profi::handlers::UsersHandler>()
            .Append<profi::handlers::ServicesHandler>()
            .Append<profi::handlers::ServiceByIdHandler>()
            .Append<profi::handlers::OrdersHandler>()
            .Append<profi::handlers::OrderByIdHandler>()
            .Append<profi::handlers::OrderServicesHandler>()
            .Append<profi::handlers::SwaggerUiHandler>()
            .Append<profi::handlers::OpenApiHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
