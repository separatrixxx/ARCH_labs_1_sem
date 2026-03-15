#pragma once

#include <algorithm>
#include <atomic>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

namespace profi {

struct User {
    std::string id;
    std::string login;
    std::string first_name;
    std::string last_name;
    std::string password_hash;
};

struct Service {
    std::string id;
    std::string title;
    std::string description;
    double price{0.0};
    std::string provider_id;
};

struct Order {
    std::string id;
    std::string client_id;
    std::vector<std::string> service_ids;
    std::string status{"pending"};
};

class StorageComponent final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "storage-component";

    StorageComponent(const userver::components::ComponentConfig& config,
                     const userver::components::ComponentContext& context)
        : ComponentBase(config, context) {}

    std::optional<User> FindUserByLogin(const std::string& login) const;
    std::optional<User> FindUserById(const std::string& id) const;
    std::vector<User> FindUsersByNameMask(const std::string& first_name,
                                          const std::string& last_name) const;
    User CreateUser(const std::string& login, const std::string& first_name,
                    const std::string& last_name,
                    const std::string& password_hash);

    Service CreateService(const std::string& title,
                          const std::string& description, double price,
                          const std::string& provider_id);
    std::vector<Service> GetAllServices() const;
    std::optional<Service> FindServiceById(const std::string& id) const;

    Order CreateOrder(const std::string& client_id);
    std::optional<Order> FindOrderById(const std::string& id) const;
    std::optional<Order> AddServiceToOrder(const std::string& order_id,
                                           const std::string& service_id);
    std::vector<Order> GetOrdersByClientId(const std::string& client_id) const;

private:
    std::string NextId() { return std::to_string(++counter_); }

    mutable std::shared_mutex mutex_;
    std::atomic<int> counter_{0};

    std::unordered_map<std::string, User> users_by_id_;
    std::unordered_map<std::string, std::string> login_to_id_;

    std::unordered_map<std::string, Service> services_;
    std::unordered_map<std::string, Order> orders_;
};

}  // namespace profi
