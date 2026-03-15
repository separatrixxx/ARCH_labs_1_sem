#include "storage.hpp"
#include <mutex>

namespace profi {

std::optional<User> StorageComponent::FindUserByLogin(
    const std::string& login) const {
    std::shared_lock lock(mutex_);
    auto it = login_to_id_.find(login);
    if (it == login_to_id_.end()) return std::nullopt;
    auto uit = users_by_id_.find(it->second);
    if (uit == users_by_id_.end()) return std::nullopt;
    return uit->second;
}

std::optional<User> StorageComponent::FindUserById(
    const std::string& id) const {
    std::shared_lock lock(mutex_);
    auto it = users_by_id_.find(id);
    if (it == users_by_id_.end()) return std::nullopt;
    return it->second;
}

std::vector<User> StorageComponent::FindUsersByNameMask(
    const std::string& first_name, const std::string& last_name) const {
    std::shared_lock lock(mutex_);
    std::vector<User> result;
    for (const auto& [id, user] : users_by_id_) {
        bool match = true;
        if (!first_name.empty())
            match = match && user.first_name.find(first_name) != std::string::npos;
        if (!last_name.empty())
            match = match && user.last_name.find(last_name) != std::string::npos;
        if (match) result.push_back(user);
    }
    return result;
}

User StorageComponent::CreateUser(const std::string& login,
                                   const std::string& first_name,
                                   const std::string& last_name,
                                   const std::string& password_hash) {
    std::unique_lock lock(mutex_);
    auto id = NextId();
    User user{id, login, first_name, last_name, password_hash};
    users_by_id_[id] = user;
    login_to_id_[login] = id;
    return user;
}

Service StorageComponent::CreateService(const std::string& title,
                                         const std::string& description,
                                         double price,
                                         const std::string& provider_id) {
    std::unique_lock lock(mutex_);
    auto id = NextId();
    Service svc{id, title, description, price, provider_id};
    services_[id] = svc;
    return svc;
}

std::vector<Service> StorageComponent::GetAllServices() const {
    std::shared_lock lock(mutex_);
    std::vector<Service> result;
    result.reserve(services_.size());
    for (const auto& [id, svc] : services_) result.push_back(svc);
    return result;
}

std::optional<Service> StorageComponent::FindServiceById(
    const std::string& id) const {
    std::shared_lock lock(mutex_);
    auto it = services_.find(id);
    if (it == services_.end()) return std::nullopt;
    return it->second;
}

Order StorageComponent::CreateOrder(const std::string& client_id) {
    std::unique_lock lock(mutex_);
    auto id = NextId();
    Order order{id, client_id, {}, "pending"};
    orders_[id] = order;
    return order;
}

std::optional<Order> StorageComponent::FindOrderById(
    const std::string& id) const {
    std::shared_lock lock(mutex_);
    auto it = orders_.find(id);
    if (it == orders_.end()) return std::nullopt;
    return it->second;
}

std::optional<Order> StorageComponent::AddServiceToOrder(
    const std::string& order_id, const std::string& service_id) {
    std::unique_lock lock(mutex_);
    auto it = orders_.find(order_id);
    if (it == orders_.end()) return std::nullopt;
    it->second.service_ids.push_back(service_id);
    return it->second;
}

std::vector<Order> StorageComponent::GetOrdersByClientId(
    const std::string& client_id) const {
    std::shared_lock lock(mutex_);
    std::vector<Order> result;
    for (const auto& [id, order] : orders_) {
        if (order.client_id == client_id) result.push_back(order);
    }
    return result;
}

}  // namespace profi
