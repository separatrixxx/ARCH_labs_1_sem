#include "storage.hpp"

#include <algorithm>
#include <cctype>

namespace profi {

namespace {

std::string ToLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

}

std::optional<User> UserStorage::FindByLogin(const std::string& login) const {
    std::shared_lock lock(mutex_);
    auto it = login_to_id_.find(login);
    if (it == login_to_id_.end()) {
        return std::nullopt;
    }

    auto uit = users_by_id_.find(it->second);
    if (uit == users_by_id_.end()) {
        return std::nullopt;
    }

    return uit->second;
}

std::optional<User> UserStorage::FindById(const std::string& id) const {
    std::shared_lock lock(mutex_);
    auto it = users_by_id_.find(id);
    if (it == users_by_id_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::vector<User> UserStorage::FindAll() const {
    std::shared_lock lock(mutex_);
    std::vector<User> result;

    result.reserve(users_by_id_.size());

    for (const auto& [id, user] : users_by_id_) {
        result.push_back(user);
    }
    
    return result;
}

std::vector<User> UserStorage::FindByNameMask(
    const std::string& first_name, const std::string& last_name) const {
    std::shared_lock lock(mutex_);
    std::vector<User> result;
    for (const auto& [id, user] : users_by_id_) {
        bool match = true;

        if (!first_name.empty()) {
            match = match && ToLower(user.first_name).find(ToLower(first_name)) != std::string::npos;
        }

        if (!last_name.empty()) {
            match = match && ToLower(user.last_name).find(ToLower(last_name)) != std::string::npos;
        }

        if (match) {
            result.push_back(user);
        }
    }

    return result;
}

User UserStorage::Create(const std::string& login, const std::string& email,
                          const std::string& first_name,
                          const std::string& last_name,
                          const std::string& password_hash) {
    std::unique_lock lock(mutex_);
    auto id = NextId();
    User user{id, login, email, first_name, last_name, password_hash};
    users_by_id_[id] = user;
    login_to_id_[login] = id;
    return user;
}

}
