#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

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
                 const userver::components::ComponentContext& context)
        : ComponentBase(config, context) {}

    Order Create(const std::string& client_id);
    std::optional<Order> FindById(const std::string& id) const;
    std::optional<Order> AddService(const std::string& order_id,
                                    const std::string& service_id);
    std::vector<Order> FindByClientId(const std::string& client_id) const;

private:
    std::string NextId() { return std::to_string(++counter_); }

    mutable std::shared_mutex mutex_;
    std::atomic<int> counter_{0};
    std::unordered_map<std::string, Order> orders_;
};

}
