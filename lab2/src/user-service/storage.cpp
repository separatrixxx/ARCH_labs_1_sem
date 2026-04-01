#include "storage.hpp"

#ifndef USE_POSTGRESQL
#include <algorithm>
#include <cctype>
#endif

namespace profi {

#ifdef USE_POSTGRESQL

namespace pg = userver::storages::postgres;

namespace {

User RowToUser(const pg::Row& row) {
    return User{
        std::to_string(row["id"].As<int64_t>()),
        row["login"].As<std::string>(),
        row["email"].As<std::string>(),
        row["first_name"].As<std::string>(),
        row["last_name"].As<std::string>(),
        row["password_hash"].As<std::string>(),
    };
}

}

UserStorage::UserStorage(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db")
              .GetCluster()) {}

std::optional<User> UserStorage::FindByLogin(const std::string& login) const {
    auto res = pg_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id, login, email, first_name, last_name, password_hash "
        "FROM users WHERE login = $1",
        login);
    if (res.IsEmpty()) return std::nullopt;
    return RowToUser(res.Front());
}

std::optional<User> UserStorage::FindById(const std::string& id) const {
    auto res = pg_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id, login, email, first_name, last_name, password_hash "
        "FROM users WHERE id = $1",
        std::stoll(id));
    if (res.IsEmpty()) return std::nullopt;
    return RowToUser(res.Front());
}

std::vector<User> UserStorage::FindAll() const {
    auto res = pg_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id, login, email, first_name, last_name, password_hash "
        "FROM users ORDER BY id");
    std::vector<User> result;
    result.reserve(res.Size());
    for (auto row : res) result.push_back(RowToUser(row));
    return result;
}

std::vector<User> UserStorage::FindByNameMask(
    const std::string& first_name, const std::string& last_name) const {
    const std::string fn = "%" + first_name + "%";
    const std::string ln = "%" + last_name + "%";
    auto res = pg_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id, login, email, first_name, last_name, password_hash "
        "FROM users "
        "WHERE ($1 = '%%' OR lower(first_name) LIKE lower($1)) "
        "  AND ($2 = '%%' OR lower(last_name)  LIKE lower($2))",
        fn, ln);
    std::vector<User> result;
    result.reserve(res.Size());
    for (auto row : res) result.push_back(RowToUser(row));
    return result;
}

User UserStorage::Create(const std::string& login, const std::string& email,
                          const std::string& first_name,
                          const std::string& last_name,
                          const std::string& password_hash) {
    auto res = pg_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO users (login, email, first_name, last_name, password_hash) "
        "VALUES ($1, $2, $3, $4, $5) "
        "RETURNING id, login, email, first_name, last_name, password_hash",
        login, email, first_name, last_name, password_hash);
    return RowToUser(res.Front());
}

#else

namespace {

std::string ToLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

}

UserStorage::UserStorage(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
    : ComponentBase(config, context) {}

std::optional<User> UserStorage::FindByLogin(const std::string& login) const {
    std::shared_lock lock(mutex_);
    auto it = login_to_id_.find(login);
    if (it == login_to_id_.end()) return std::nullopt;
    auto uit = users_by_id_.find(it->second);
    if (uit == users_by_id_.end()) return std::nullopt;
    return uit->second;
}

std::optional<User> UserStorage::FindById(const std::string& id) const {
    std::shared_lock lock(mutex_);
    auto it = users_by_id_.find(id);
    if (it == users_by_id_.end()) return std::nullopt;
    return it->second;
}

std::vector<User> UserStorage::FindAll() const {
    std::shared_lock lock(mutex_);
    std::vector<User> result;
    result.reserve(users_by_id_.size());
    for (const auto& [id, user] : users_by_id_) result.push_back(user);
    return result;
}

std::vector<User> UserStorage::FindByNameMask(
    const std::string& first_name, const std::string& last_name) const {
    std::shared_lock lock(mutex_);
    std::vector<User> result;
    for (const auto& [id, user] : users_by_id_) {
        bool match = true;
        if (!first_name.empty())
            match = match && ToLower(user.first_name).find(ToLower(first_name)) != std::string::npos;
        if (!last_name.empty())
            match = match && ToLower(user.last_name).find(ToLower(last_name)) != std::string::npos;
        if (match) result.push_back(user);
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

#endif

}
