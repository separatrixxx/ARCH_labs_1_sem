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

struct Order {
    std::string id;
    std::string client_id;
    std::vector<std::string> service_ids;
    std::string status{"pending"};
};

class OrderStorage final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "order-storage";

    OrderStorage(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);

    Order Create(const std::string& client_id);
    std::optional<Order> FindById(const std::string& id) const;
    std::optional<Order> AddService(const std::string& order_id,
                                    const std::string& service_id);
    std::vector<Order> FindByClientId(const std::string& client_id) const;

private:
#ifdef USE_POSTGRESQL
    Order FetchOrder(const std::string& id) const;

    userver::storages::postgres::ClusterPtr pg_;
#elif defined(USE_MONGODB)
    Order FetchOrder(const std::string& id) const;

    userver::storages::mongo::PoolPtr mongo_;
#else
    std::string NextId() { return std::to_string(++counter_); }

    mutable std::shared_mutex mutex_;
    std::atomic<int> counter_{0};
    std::unordered_map<std::string, Order> orders_;
#endif
};

}
