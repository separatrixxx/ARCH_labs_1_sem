#include "storage.hpp"

namespace profi {

Order OrderStorage::Create(const std::string& client_id) {
    std::unique_lock lock(mutex_);
    auto id = NextId();
    Order order{id, client_id, {}, "pending"};
    orders_[id] = order;
    return order;
}

std::optional<Order> OrderStorage::FindById(const std::string& id) const {
    std::shared_lock lock(mutex_);
    auto it = orders_.find(id);
    if (it == orders_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::optional<Order> OrderStorage::AddService(const std::string& order_id,
                                               const std::string& service_id) {
    std::unique_lock lock(mutex_);
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return std::nullopt;
    }
    it->second.service_ids.push_back(service_id);
    return it->second;
}

std::vector<Order> OrderStorage::FindByClientId(
    const std::string& client_id) const {
    std::shared_lock lock(mutex_);
    std::vector<Order> result;
    for (const auto& [id, order] : orders_) {
        if (order.client_id == client_id) {
            result.push_back(order);
        }
    }

    return result;
}

}
