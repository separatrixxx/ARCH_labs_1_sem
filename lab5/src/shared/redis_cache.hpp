#pragma once

#include <mutex>
#include <optional>
#include <string>

#include <hiredis/hiredis.h>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

namespace profi {

class RedisCache final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "redis-cache";

    RedisCache(const userver::components::ComponentConfig& config,
               const userver::components::ComponentContext& context);
    ~RedisCache() override;

    std::optional<std::string> Get(const std::string& key) const;
    void Set(const std::string& key, const std::string& value,
             int ttl_seconds) const;
    void Del(const std::string& key) const;

    struct RateLimitResult {
        bool allowed;
        int64_t limit;
        int64_t remaining;
        int64_t reset_after;
    };

    RateLimitResult CheckRateLimit(const std::string& key,
                                   int64_t max_requests,
                                   int window_seconds) const;

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    void EnsureConnected() const;

    std::string host_;
    int port_;
    mutable std::mutex mutex_;
    mutable redisContext* ctx_{nullptr};
};

}
