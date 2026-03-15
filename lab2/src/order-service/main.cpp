#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include "storage.hpp"
#include "handlers/orders_handler.hpp"
#include "auth_middleware.hpp"

int main(int argc, char* argv[]) {
    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<profi::OrderStorage>()
            .Append<profi::JwtAuthMiddlewareFactory>()
            .Append<userver::components::HttpClient>()
            .Append<profi::handlers::OrdersHandler>()
            .Append<profi::handlers::OrderByIdHandler>()
            .Append<profi::handlers::OrderServicesHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
