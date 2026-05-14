#include "redis_cache.hpp"

#include <userver/logging/log.hpp>

namespace profi {

RedisCache::RedisCache(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& ctx)
    : LoggableComponentBase(config, ctx),
      host_(config["host"].As<std::string>("redis")),
      port_(config["port"].As<int>(6379)) {
    EnsureConnected();
    if (ctx_)
        LOG_INFO() << "Redis connected: " << host_ << ":" << port_;
}

RedisCache::~RedisCache() {
    std::lock_guard lock(mutex_);
    if (ctx_) {
        redisFree(ctx_);
        ctx_ = nullptr;
    }
}

void RedisCache::EnsureConnected() const {
    if (ctx_ && !ctx_->err) return;

    if (ctx_) {
        redisFree(ctx_);
        ctx_ = nullptr;
    }

    struct timeval timeout = {1, 500000};
    ctx_ = redisConnectWithTimeout(host_.c_str(), port_, timeout);

    if (!ctx_ || ctx_->err) {
        LOG_WARNING() << "Redis connection failed: "
                      << (ctx_ ? ctx_->errstr : "allocation error");
        if (ctx_) {
            redisFree(ctx_);
            ctx_ = nullptr;
        }
    }
}

std::optional<std::string> RedisCache::Get(const std::string& key) const {
    std::lock_guard lock(mutex_);
    EnsureConnected();
    if (!ctx_) return std::nullopt;

    auto* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "GET %b", key.data(), key.size()));

    if (!reply) {
        redisFree(ctx_);
        ctx_ = nullptr;
        return std::nullopt;
    }

    std::optional<std::string> result;
    if (reply->type == REDIS_REPLY_STRING) {
        result.emplace(reply->str, reply->len);
    }
    freeReplyObject(reply);
    return result;
}

void RedisCache::Set(const std::string& key, const std::string& value,
                     int ttl_seconds) const {
    std::lock_guard lock(mutex_);
    EnsureConnected();
    if (!ctx_) return;

    auto* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "SET %b %b EX %d", key.data(), key.size(), value.data(),
        value.size(), ttl_seconds));

    if (reply) {
        freeReplyObject(reply);
    } else {
        redisFree(ctx_);
        ctx_ = nullptr;
    }
}

void RedisCache::Del(const std::string& key) const {
    std::lock_guard lock(mutex_);
    EnsureConnected();
    if (!ctx_) return;

    auto* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "DEL %b", key.data(), key.size()));

    if (reply) {
        freeReplyObject(reply);
    } else {
        redisFree(ctx_);
        ctx_ = nullptr;
    }
}

RedisCache::RateLimitResult RedisCache::CheckRateLimit(
    const std::string& key, int64_t max_requests,
    int window_seconds) const {
    std::lock_guard lock(mutex_);
    EnsureConnected();

    if (!ctx_) {
        return {true, max_requests, max_requests - 1, window_seconds};
    }

    auto* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "INCR %b", key.data(), key.size()));

    if (!reply) {
        redisFree(ctx_);
        ctx_ = nullptr;
        return {true, max_requests, max_requests - 1, window_seconds};
    }

    int64_t count = reply->integer;
    freeReplyObject(reply);

    if (count == 1) {
        reply = static_cast<redisReply*>(
            redisCommand(ctx_, "EXPIRE %b %d", key.data(), key.size(),
                         window_seconds));
        if (reply) freeReplyObject(reply);
    }

    reply = static_cast<redisReply*>(
        redisCommand(ctx_, "TTL %b", key.data(), key.size()));
    int64_t ttl = window_seconds;
    if (reply) {
        if (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0) {
            ttl = reply->integer;
        }
        freeReplyObject(reply);
    }

    bool allowed = count <= max_requests;
    int64_t remaining = allowed ? (max_requests - count) : 0;

    return {allowed, max_requests, remaining, ttl};
}

userver::yaml_config::Schema RedisCache::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<
        userver::components::LoggableComponentBase>(R"(
type: object
description: Redis client (cache + rate limiting)
additionalProperties: false
properties:
    host:
        type: string
        description: hostname
    port:
        type: integer
        description: port
)");
}

}
