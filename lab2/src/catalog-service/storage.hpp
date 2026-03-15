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
                   const userver::components::ComponentContext& context)
        : ComponentBase(config, context) {}

    Service Create(const std::string& title, const std::string& description,
                   double price, const std::string& provider_id);
    std::vector<Service> GetAll() const;
    std::optional<Service> FindById(const std::string& id) const;

private:
    std::string NextId() { return std::to_string(++counter_); }

    mutable std::shared_mutex mutex_;
    std::atomic<int> counter_{0};
    std::unordered_map<std::string, Service> services_;
};

}  // namespace profi
