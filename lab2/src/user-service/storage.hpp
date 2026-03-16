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

struct User {
    std::string id;
    std::string login;
    std::string email;
    std::string first_name;
    std::string last_name;
    std::string password_hash;
};

class UserStorage final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "user-storage";

    UserStorage(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context)
        : ComponentBase(config, context) {}

    std::optional<User> FindByLogin(const std::string& login) const;
    std::optional<User> FindById(const std::string& id) const;
    std::vector<User> FindAll() const;
    std::vector<User> FindByNameMask(const std::string& first_name,
                                     const std::string& last_name) const;
    User Create(const std::string& login, const std::string& email,
                const std::string& first_name, const std::string& last_name,
                const std::string& password_hash);

private:
    std::string NextId() { return std::to_string(++counter_); }

    mutable std::shared_mutex mutex_;
    std::atomic<int> counter_{0};
    std::unordered_map<std::string, User> users_by_id_;
    std::unordered_map<std::string, std::string> login_to_id_;
};

}
