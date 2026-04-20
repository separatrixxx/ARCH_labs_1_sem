#include "storage.hpp"

#ifdef USE_MONGODB
#include <chrono>
#include <userver/storages/mongo/component.hpp>
#include <userver/formats/bson/serialize.hpp>
#include <userver/formats/bson/types.hpp>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/operations.hpp>
#include <userver/storages/mongo/options.hpp>
#endif

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

#elif defined(USE_MONGODB)

namespace mg = userver::storages::mongo;
namespace fb = userver::formats::bson;
namespace mo = userver::storages::mongo::options;

namespace {

Service DocToService(const fb::Value& doc) {
    std::string description;
    if (!doc["description"].IsMissing() && doc["description"].IsString())
        description = doc["description"].As<std::string>();
    return Service{
        doc["_id"].As<fb::Oid>().ToString(),
        doc["title"].As<std::string>(),
        description,
        doc["price"].As<double>(),
        doc["provider_id"].As<std::string>(),
    };
}

fb::Value EmptyDoc() {
    return fb::ValueBuilder(userver::formats::common::Type::kObject).ExtractValue();
}

}

CatalogStorage::CatalogStorage(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      mongo_(context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()) {}

Service CatalogStorage::Create(const std::string& title,
                                const std::string& description, double price,
                                const std::string& provider_id) {
    fb::Oid oid{};
    fb::ValueBuilder doc;
    doc["_id"]         = oid;
    doc["title"]       = title;
    doc["description"] = description;
    doc["price"]       = price;
    doc["provider_id"] = provider_id;
    doc["created_at"]  = std::chrono::system_clock::now();
    auto coll = mongo_->GetCollection("services");
    coll.Execute(mg::operations::InsertOne(doc.ExtractValue()));
    return Service{oid.ToString(), title, description, price, provider_id};
}

std::vector<Service> CatalogStorage::GetAll() const {
    auto coll = mongo_->GetCollection("services");
    mg::operations::Find find(EmptyDoc());
    find.SetOption(mo::Sort{}.By("_id", mo::Sort::Direction::kAscending));
    std::vector<Service> result;
    for (const auto& doc : coll.Execute(find)) result.push_back(DocToService(doc));
    return result;
}

std::optional<Service> CatalogStorage::FindById(const std::string& id) const {
    try {
        fb::Oid oid(id);
        fb::ValueBuilder filter_builder;
        filter_builder["_id"] = oid;
        auto coll = mongo_->GetCollection("services");
        mg::operations::Find find(filter_builder.ExtractValue());
        find.SetOption(mo::Limit{1});
        for (const auto& doc : coll.Execute(find)) return DocToService(doc);
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
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
