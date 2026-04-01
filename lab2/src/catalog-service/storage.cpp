#include "storage.hpp"

namespace profi {

#ifdef USE_POSTGRESQL

namespace pg = userver::storages::postgres;

namespace {

Service RowToService(const pg::Row& row) {
    return Service{
        std::to_string(row["id"].As<int64_t>()),
        row["title"].As<std::string>(),
        row["description"].As<std::optional<std::string>>().value_or(""),
        row["price"].As<double>(),
        std::to_string(row["provider_id"].As<int64_t>()),
    };
}

}

CatalogStorage::CatalogStorage(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db")
              .GetCluster()) {}

Service CatalogStorage::Create(const std::string& title,
                                const std::string& description, double price,
                                const std::string& provider_id) {
    auto res = pg_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO services (title, description, price, provider_id) "
        "VALUES ($1, $2, $3, $4) "
        "RETURNING id, title, description, price, provider_id",
        title, description, price, std::stoll(provider_id));
    return RowToService(res.Front());
}

std::vector<Service> CatalogStorage::GetAll() const {
    auto res = pg_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id, title, description, price, provider_id "
        "FROM services ORDER BY id");
    std::vector<Service> result;
    result.reserve(res.Size());
    for (auto row : res) result.push_back(RowToService(row));
    return result;
}

std::optional<Service> CatalogStorage::FindById(const std::string& id) const {
    auto res = pg_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id, title, description, price, provider_id "
        "FROM services WHERE id = $1",
        std::stoll(id));
    if (res.IsEmpty()) return std::nullopt;
    return RowToService(res.Front());
}

#else

CatalogStorage::CatalogStorage(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context) {}

Service CatalogStorage::Create(const std::string& title,
                                const std::string& description, double price,
                                const std::string& provider_id) {
    std::unique_lock lock(mutex_);
    auto id = NextId();
    Service svc{id, title, description, price, provider_id};
    services_[id] = svc;
    return svc;
}

std::vector<Service> CatalogStorage::GetAll() const {
    std::shared_lock lock(mutex_);
    std::vector<Service> result;
    result.reserve(services_.size());
    for (const auto& [id, svc] : services_) result.push_back(svc);
    return result;
}

std::optional<Service> CatalogStorage::FindById(const std::string& id) const {
    std::shared_lock lock(mutex_);
    auto it = services_.find(id);
    if (it == services_.end()) return std::nullopt;
    return it->second;
}

#endif

}
