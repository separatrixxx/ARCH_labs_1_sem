#include "storage.hpp"

namespace profi {

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
    for (const auto& [id, svc] : services_) {
        result.push_back(svc);
    }

    return result;
}

std::optional<Service> CatalogStorage::FindById(const std::string& id) const {
    std::shared_lock lock(mutex_);
    auto it = services_.find(id);
    if (it == services_.end()) {
        return std::nullopt;
    }

    return it->second;
}

}
