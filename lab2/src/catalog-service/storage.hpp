#pragma once

#include <optional>
#include <string>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#ifdef USE_POSTGRESQL
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#elif defined(USE_MONGODB)
#include <userver/storages/mongo/pool.hpp>
#else
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#endif

namespace profi {

struct Service {
    std::string id;
    std::string title;
    std::string description;
    double price{0.0};
    std::string provider_id;
};

class CatalogStorage final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "catalog-storage";

    CatalogStorage(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

    Service Create(const std::string& title, const std::string& description,
                   double price, const std::string& provider_id);
    std::vector<Service> GetAll() const;
    std::optional<Service> FindById(const std::string& id) const;

private:
#ifdef USE_POSTGRESQL
    userver::storages::postgres::ClusterPtr pg_;
#elif defined(USE_MONGODB)
    userver::storages::mongo::PoolPtr mongo_;
#else
    std::string NextId() { return std::to_string(++counter_); }

    mutable std::shared_mutex mutex_;
    std::atomic<int> counter_{0};
    std::unordered_map<std::string, Service> services_;
#endif
};

}
