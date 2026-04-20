#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#ifdef USE_POSTGRESQL
#include <userver/clients/dns/component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#elif defined(USE_MONGODB)
#include <userver/clients/dns/component.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#endif

#include "storage.hpp"
#include "handlers/services_handler.hpp"
#include "auth_middleware.hpp"

int main(int argc, char* argv[]) {
    auto component_list =
        userver::components::MinimalServerComponentList()
#ifdef USE_POSTGRESQL
            .Append<userver::components::TestsuiteSupport>()
            .Append<userver::components::Postgres>("postgres-db")
            .Append<userver::clients::dns::Component>()
#elif defined(USE_MONGODB)
            .Append<userver::components::TestsuiteSupport>()
            .Append<userver::components::Mongo>("mongo-db")
            .Append<userver::clients::dns::Component>()
#endif
            .Append<profi::CatalogStorage>()
            .Append<profi::JwtAuthMiddlewareFactory>()
            .Append<profi::handlers::ServicesHandler>()
            .Append<profi::handlers::ServiceByIdHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
