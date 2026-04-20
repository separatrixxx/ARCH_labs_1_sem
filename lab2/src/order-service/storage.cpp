#include "storage.hpp"

#ifdef USE_MONGODB
#include <chrono>
#include <stdexcept>
#include <userver/components/mongo.hpp>
#include <userver/formats/bson/serialize.hpp>
#include <userver/formats/bson/types.hpp>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/operations.hpp>
#endif

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
#ifdef SERVICE_ID_TEXT
        order.service_ids.push_back(row["service_id"].As<std::string>());
#else
        order.service_ids.push_back(std::to_string(row["service_id"].As<int64_t>()));
#endif
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

#ifdef SERVICE_ID_TEXT
    pg_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO order_services (order_id, service_id) VALUES ($1, $2) "
        "ON CONFLICT DO NOTHING",
        std::stoll(order_id), service_id);
#else
    pg_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO order_services (order_id, service_id) VALUES ($1, $2) "
        "ON CONFLICT DO NOTHING",
        std::stoll(order_id), std::stoll(service_id));
#endif

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

#elif defined(USE_MONGODB)

namespace mg = userver::storages::mongo;
namespace fb = userver::formats::bson;

namespace {

Order DocToOrder(const fb::Value& doc) {
    Order order;
    order.id        = doc["_id"].As<fb::Oid>().ToHexString();
    order.client_id = doc["client_id"].As<std::string>();
    order.status    = doc["status"].As<std::string>("pending");
    if (!doc["service_ids"].IsMissing()) {
        for (const auto& v : doc["service_ids"])
            order.service_ids.push_back(v.As<std::string>());
    }
    return order;
}

}

OrderStorage::OrderStorage(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      mongo_(context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()) {}

Order OrderStorage::FetchOrder(const std::string& id) const {
    fb::Oid oid(id);
    auto coll = mongo_->GetCollection("orders");
    mg::operations::Find find(fb::MakeDoc("_id", oid));
    find.SetLimit(1);
    for (const auto& doc : coll.Execute(find)) return DocToOrder(doc);
    throw std::runtime_error("Order not found: " + id);
}

Order OrderStorage::Create(const std::string& client_id) {
    fb::Oid oid{};
    fb::ValueBuilder doc;
    doc["_id"]         = oid;
    doc["client_id"]   = client_id;
    doc["status"]      = std::string("pending");
    doc["service_ids"] = fb::MakeArray();
    doc["created_at"]  = std::chrono::system_clock::now();
    auto coll = mongo_->GetCollection("orders");
    coll.Execute(mg::operations::InsertOne(doc.ExtractValue()));
    return FetchOrder(oid.ToHexString());
}

std::optional<Order> OrderStorage::FindById(const std::string& id) const {
    try {
        fb::Oid oid(id);
        auto coll = mongo_->GetCollection("orders");
        mg::operations::Find find(fb::MakeDoc("_id", oid));
        find.SetLimit(1);
        bool found = false;
        for (const auto& _ : coll.Execute(find)) { found = true; }
        if (!found) return std::nullopt;
        return FetchOrder(id);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Order> OrderStorage::AddService(const std::string& order_id,
                                               const std::string& service_id) {
    try {
        fb::Oid oid(order_id);
        auto coll = mongo_->GetCollection("orders");
        mg::operations::Find check(fb::MakeDoc("_id", oid));
        check.SetLimit(1);
        bool found = false;
        for (const auto& _ : coll.Execute(check)) { found = true; }
        if (!found) return std::nullopt;
        mg::operations::UpdateOne upd(
            fb::MakeDoc("_id", oid),
            fb::MakeDoc("$addToSet", fb::MakeDoc("service_ids", service_id)));
        coll.Execute(upd);
        return FetchOrder(order_id);
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<Order> OrderStorage::FindByClientId(
    const std::string& client_id) const {
    auto coll = mongo_->GetCollection("orders");
    mg::operations::Find find(fb::MakeDoc("client_id", client_id));
    find.SetSort(fb::MakeDoc("_id", 1));
    std::vector<Order> result;
    for (const auto& doc : coll.Execute(find)) result.push_back(DocToOrder(doc));
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
