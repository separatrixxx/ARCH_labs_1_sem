#include "storage.hpp"

namespace profi {

#ifdef USE_POSTGRESQL

namespace pg = userver::storages::postgres;

OrderStorage::OrderStorage(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db")
              .GetCluster()) {}

Order OrderStorage::FetchOrder(const std::string& id) const {
    const auto order_id = std::stoll(id);

    auto order_res = pg_->Execute(
        pg::ClusterHostType::kMaster,
        "SELECT id, client_id, status FROM orders WHERE id = $1",
        order_id);

    auto svc_res = pg_->Execute(
        pg::ClusterHostType::kMaster,
        "SELECT service_id FROM order_services WHERE order_id = $1 ORDER BY service_id",
        order_id);

    Order order;
    order.id        = std::to_string(order_res.Front()["id"].As<int64_t>());
    order.client_id = std::to_string(order_res.Front()["client_id"].As<int64_t>());
    order.status    = order_res.Front()["status"].As<std::string>();
    for (auto row : svc_res)
        order.service_ids.push_back(std::to_string(row["service_id"].As<int64_t>()));
    return order;
}

Order OrderStorage::Create(const std::string& client_id) {
    auto res = pg_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO orders (client_id) VALUES ($1) RETURNING id",
        std::stoll(client_id));
    const auto new_id = std::to_string(res.Front()["id"].As<int64_t>());
    return FetchOrder(new_id);
}

std::optional<Order> OrderStorage::FindById(const std::string& id) const {
    auto check = pg_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT 1 FROM orders WHERE id = $1",
        std::stoll(id));
    if (check.IsEmpty()) return std::nullopt;
    return FetchOrder(id);
}

std::optional<Order> OrderStorage::AddService(const std::string& order_id,
                                               const std::string& service_id) {
    auto check = pg_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT 1 FROM orders WHERE id = $1",
        std::stoll(order_id));
    if (check.IsEmpty()) return std::nullopt;

    pg_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO order_services (order_id, service_id) VALUES ($1, $2) "
        "ON CONFLICT DO NOTHING",
        std::stoll(order_id), std::stoll(service_id));

    return FetchOrder(order_id);
}

std::vector<Order> OrderStorage::FindByClientId(
    const std::string& client_id) const {
    auto order_res = pg_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id FROM orders WHERE client_id = $1 ORDER BY id",
        std::stoll(client_id));

    std::vector<Order> result;
    result.reserve(order_res.Size());
    for (auto row : order_res) {
        const auto oid = std::to_string(row["id"].As<int64_t>());
        result.push_back(FetchOrder(oid));
    }
    return result;
}

#else

OrderStorage::OrderStorage(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context) {}

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
    if (it == orders_.end()) return std::nullopt;
    return it->second;
}

std::optional<Order> OrderStorage::AddService(const std::string& order_id,
                                               const std::string& service_id) {
    std::unique_lock lock(mutex_);
    auto it = orders_.find(order_id);
    if (it == orders_.end()) return std::nullopt;
    it->second.service_ids.push_back(service_id);
    return it->second;
}

std::vector<Order> OrderStorage::FindByClientId(
    const std::string& client_id) const {
    std::shared_lock lock(mutex_);
    std::vector<Order> result;
    for (const auto& [id, order] : orders_) {
        if (order.client_id == client_id) result.push_back(order);
    }
    return result;
}

#endif

}
