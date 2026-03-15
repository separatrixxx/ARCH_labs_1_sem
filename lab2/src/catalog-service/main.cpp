#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include "storage.hpp"
#include "handlers/services_handler.hpp"
#include "auth_middleware.hpp"

int main(int argc, char* argv[]) {
    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<profi::CatalogStorage>()
            .Append<profi::JwtAuthMiddlewareFactory>()
            .Append<profi::handlers::ServicesHandler>()
            .Append<profi::handlers::ServiceByIdHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
