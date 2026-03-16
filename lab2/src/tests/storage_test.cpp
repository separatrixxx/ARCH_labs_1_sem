#include <userver/utest/utest.hpp>

#include "storage.hpp"
#include "../catalog-service/storage.hpp"
#include "../order-service/storage.hpp"
#include "jwt.hpp"

namespace {

struct TestStorage {
    profi::User CreateUser(const std::string& login,
                            const std::string& first_name,
                            const std::string& last_name,
                            const std::string& pw_hash) {
        auto id = std::to_string(++counter_);
        profi::User u{id, login, /*email=*/"", first_name, last_name, pw_hash};
        users_[id] = u;
        login_to_id_[login] = id;
        return u;
    }

    std::optional<profi::User> FindUserByLogin(const std::string& login) const {
        auto it = login_to_id_.find(login);

        if (it == login_to_id_.end()) {
            return std::nullopt;
        }

        auto uit = users_.find(it->second);

        if (uit == users_.end()) {
            return std::nullopt;
        }
        
        return uit->second;
    }

    std::vector<profi::User> FindByNameMask(const std::string& fn,
                                              const std::string& ln) const {
        std::vector<profi::User> res;
        for (const auto& [id, u] : users_) {
            bool ok = true;
            if (!fn.empty()) {
                ok = ok && u.first_name.find(fn) != std::string::npos;
            }
            if (!ln.empty()) {
                ok = ok && u.last_name.find(ln) != std::string::npos;
            }
            if (ok) {
                res.push_back(u);
            }
        }
        return res;
    }

    profi::Service CreateService(const std::string& title,
                                   const std::string& desc, double price,
                                   const std::string& provider_id) {
        auto id = std::to_string(++counter_);
        profi::Service s{id, title, desc, price, provider_id};
        services_[id] = s;
        return s;
    }

    profi::Order CreateOrder(const std::string& client_id) {
        auto id = std::to_string(++counter_);
        profi::Order o{id, client_id, {}, "pending"};
        orders_[id] = o;
        return o;
    }

    std::optional<profi::Order> AddServiceToOrder(const std::string& order_id,
                                                    const std::string& service_id) {
        auto it = orders_.find(order_id);
        if (it == orders_.end()) {
            return std::nullopt;
        }
        it->second.service_ids.push_back(service_id);
        return it->second;
    }

    int counter_{0};
    std::unordered_map<std::string, profi::User> users_;
    std::unordered_map<std::string, std::string> login_to_id_;
    std::unordered_map<std::string, profi::Service> services_;
    std::unordered_map<std::string, profi::Order> orders_;
};

}

UTEST(Storage, CreateAndFindUserByLogin) {
    TestStorage s;
    s.CreateUser("alice", "Alice", "Smith", "hash123");

    auto found = s.FindUserByLogin("alice");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->login, "alice");
    EXPECT_EQ(found->first_name, "Alice");
    EXPECT_EQ(found->last_name, "Smith");
}

UTEST(Storage, FindUserByLoginNotFound) {
    TestStorage s;
    EXPECT_FALSE(s.FindUserByLogin("nobody").has_value());
}

UTEST(Storage, FindUsersByNameMask) {
    TestStorage s;
    s.CreateUser("alice", "Alice", "Smith", "h");
    s.CreateUser("bob", "Bob", "Smith", "h");
    s.CreateUser("carol", "Carol", "Jones", "h");

    auto smiths = s.FindByNameMask("", "Smith");
    EXPECT_EQ(smiths.size(), 2u);

    auto alices = s.FindByNameMask("Alice", "");
    EXPECT_EQ(alices.size(), 1u);

    auto both = s.FindByNameMask("Alice", "Smith");
    EXPECT_EQ(both.size(), 1u);
}

UTEST(Storage, CreateService) {
    TestStorage s;
    auto svc = s.CreateService("Cleaning", "Deep clean", 1500.0, "provider1");
    EXPECT_EQ(svc.title, "Cleaning");
    EXPECT_DOUBLE_EQ(svc.price, 1500.0);
    EXPECT_EQ(svc.provider_id, "provider1");
}

UTEST(Storage, CreateOrderAndAddService) {
    TestStorage s;
    auto svc = s.CreateService("Repair", "Fix it", 500.0, "p1");
    auto order = s.CreateOrder("client1");

    EXPECT_EQ(order.service_ids.size(), 0u);
    EXPECT_EQ(order.status, "pending");

    auto updated = s.AddServiceToOrder(order.id, svc.id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->service_ids.size(), 1u);
    EXPECT_EQ(updated->service_ids[0], svc.id);
}

UTEST(Storage, AddServiceToNonExistentOrder) {
    TestStorage s;
    auto result = s.AddServiceToOrder("no-such-order", "svc1");
    EXPECT_FALSE(result.has_value());
}

UTEST(Jwt, CreateAndValidate) {
    const std::string secret = "test-secret";
    const auto token = profi::jwt::Create("user42", "alice", secret);
    EXPECT_FALSE(token.empty());

    const auto claims = profi::jwt::Validate(token, secret);
    EXPECT_EQ(claims.user_id, "user42");
    EXPECT_EQ(claims.login, "alice");
}

UTEST(Jwt, WrongSecretFails) {
    const auto token = profi::jwt::Create("user1", "bob", "secret1");
    EXPECT_THROW(profi::jwt::Validate(token, "wrong-secret"),
                 std::runtime_error);
}

UTEST(Jwt, TamperedTokenFails) {
    const auto token = profi::jwt::Create("user1", "bob", "secret");
    const auto tampered = token + "x";
    EXPECT_THROW(profi::jwt::Validate(tampered, "secret"), std::runtime_error);
}
