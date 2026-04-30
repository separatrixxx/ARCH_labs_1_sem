# Лабораторная работа 5: Оптимизация производительности через кеширование и rate limiting

## Цель работы
Получить практические навыки проектирования систем с учетом производительности, реализации кеширования и rate limiting.

## Задание

Для своего варианта задания выполните следующие задачи:

### 1. Анализ производительности
- Изучите выбранный вариант задания
- Определите операции, которые будут выполняться часто (hot paths)
- Определите операции, которые могут быть медленными (обращения к БД, внешние API)
- Определите требования к производительности (время отклика, пропускная способность)

### 2. Проектирование стратегии кеширования
- Определите, какие данные можно кешировать:
  - Часто читаемые данные
  - Результаты сложных вычислений
  - Данные, которые редко изменяются
- Выберите стратегию кеширования для каждого типа данных:
  - Cache-Aside (Lazy Loading)
  - Read-Through
  - Write-Through
  - Write-Back (Write-Behind)
- Определите TTL (Time To Live) для кешируемых данных
- Определите стратегию инвалидации кеша

### 3. Реализация кеширования
- Реализуйте простое кеширование в вашем REST API (можно использовать Redis)
- Примените кеширование минимум для 2 endpoints
- Реализуйте инвалидацию кеша при обновлении данных

### 4. Проектирование rate limiting
- Определите, какие endpoints требуют rate limiting
- Выберите алгоритм rate limiting для каждого endpoint:
  - Token Bucket
  - Leaking Bucket
  - Fixed Window Counter
  - Sliding Window Log/Counter
- Определите лимиты (например, 100 запросов в минуту для обычных пользователей, 1000 для премиум)

### 5. Реализация rate limiting
- Реализуйте простой rate limiting для минимум одного endpoint
- Используйте выбранный алгоритм
- Возвращайте правильные HTTP статус-коды (429 Too Many Requests)
- Добавление заголовков с информацией о лимитах (`X-RateLimit-Limit`, `X-RateLimit-Remaining`, `X-RateLimit-Reset`)

## 6. Анализ производительности

- Опишите, как кеширование и rate limiting улучшают производительность системы.
- Определите метрики для мониторинга производительности.
- Опишите, как измерить эффективность кеширования (hit rate).

## Результат

Результат должен быть оформлен в виде следующих файлов, размещенных в вашем GitHub репозитории:

- `performance_design.md` – описание стратегии кеширования и rate limiting.
- Исходный код с реализацией кеширования и/или rate limiting.
- `README.md` – описание проекта и реализованных оптимизаций.
- `Dockerfile` и `docker-compose.yaml` для запуска приложения.

## Критерии оценки

- Обоснованность выбора стратегий кеширования.
- Корректность проектирования rate limiting.
- Качество реализации (если реализовано).
- Анализ влияния на производительность.
- Качество документации.

---

## Реализация: Profi — маркетплейс услуг (вариант 4)

**Студент:** Лохматов Никита, М8О-107СВ-25

### Архитектура

```
                    ┌──────────┐
                    │  nginx   │ :8080
                    └────┬─────┘
            ┌────────────┼────────────┐
            ▼            ▼            ▼
     ┌────────────┐ ┌──────────┐ ┌──────────┐
     │user-service│ │ catalog  │ │  order   │
     │   :8081    │ │ service  │ │ service  │
     │ (PG+Redis) │ │:8082     │ │  :8083   │
     └──┬────┬────┘ │(Mongo+   │ │(PG+Redis)│
        │    │      │  Redis)  │ └──┬──┬────┘
        │    │      └──┬──┬───┘    │  │
        ▼    ▼         ▼  ▼        ▼  ▼
  ┌────────┐ ┌───────────────┐ ┌────────┐
  │Postgres│ │     Redis     │ │MongoDB │
  └────────┘ │  (cache + RL) │ └────────┘
             └───────────────┘
```

**Новое в lab5** по сравнению с lab4:
- **Redis 7** — добавлен как единый кеш- и rate-limit-слой для всех сервисов
- **Кеширование (Cache-Aside)** — для 3 эндпоинтов через Redis
- **Rate limiting (Fixed Window Counter)** — для auth-эндпоинтов через Redis

### Реализованное кеширование

Стратегия: **Cache-Aside (Lazy Loading)**

| Endpoint | Ключ Redis | TTL | Инвалидация |
|----------|-----------|-----|-------------|
| `GET /api/v1/services` | `services:all` | 60 с | При `POST /api/v1/services` |
| `GET /api/v1/services/{id}` | `service:{id}` | 120 с | По TTL |
| `GET /api/v1/users?login=...` | `user:login:{login}` | 30 с | По TTL |

Каждый кешированный ответ содержит заголовок `X-Cache: HIT` или `X-Cache: MISS`.

При недоступности Redis запросы проходят напрямую к БД (fail-open).

### Реализованный rate limiting

Алгоритм: **Fixed Window Counter** — атомарный `INCR` + `EXPIRE` в Redis.

| Endpoint | Лимит | Окно |
|----------|-------|------|
| `POST /api/v1/auth/login` | 10 запросов | 60 сек |
| `POST /api/v1/auth/register` | 5 запросов | 60 сек |

Ответ при превышении лимита: **HTTP 429 Too Many Requests**.

Заголовки в каждом ответе:
```
X-RateLimit-Limit: 10
X-RateLimit-Remaining: 7
X-RateLimit-Reset: 45
```

### Структура файлов

```
lab5/
├── README.md                    # этот файл
├── performance_design.md        # проектирование стратегий
├── docker-compose.yaml          # оркестрация (PG + Mongo + Redis + сервисы)
├── Dockerfile                   # сборка C++ сервисов (с hiredis)
├── Dockerfile.postgres          # PostgreSQL с инициализацией
├── Dockerfile.nginx             # nginx reverse proxy
├── CMakeLists.txt               # сборка с USE_CACHE=ON + hiredis
├── schema.sql                   # DDL PostgreSQL
├── data.sql                     # тестовые данные PostgreSQL
├── data.js                      # тестовые данные MongoDB
├── configs/
│   ├── config_vars.yaml         # переменные окружения (+ redis_host/port)
│   ├── nginx/nginx.conf
│   ├── user-service/static_config.yaml
│   ├── catalog-service/static_config.yaml
│   └── order-service/static_config.yaml
└── src/
    ├── shared/
    │   ├── redis_cache.hpp      # RedisCache userver-компонент (заголовок)
    │   └── redis_cache.cpp      # реализация: Get/Set/Del + CheckRateLimit
    ├── user-service/
    │   ├── main.cpp             # + регистрация RedisCache
    │   └── handlers/
    │       ├── auth_handler.hpp  # + redis_ member для rate limiting
    │       ├── auth_handler.cpp  # + rate limiting (login: 10/мин, register: 5/мин)
    │       ├── users_handler.hpp # + redis_ member для кеширования
    │       └── users_handler.cpp # + кеш user:login:{login} (TTL 30с)
    ├── catalog-service/
    │   ├── main.cpp             # + регистрация RedisCache
    │   └── handlers/
    │       ├── services_handler.hpp # + redis_ member
    │       └── services_handler.cpp # + кеш services:all (TTL 60с) + инвалидация
    └── order-service/
        └── main.cpp             # + регистрация RedisCache
```

### Запуск

```bash
cd lab5
docker-compose up --build
```

Сервисы будут доступны:
- API Gateway: http://localhost:8080
- Swagger UI: http://localhost:8080/swagger
- Redis CLI: `docker-compose exec redis redis-cli`

### Проверка кеширования

```bash
# Первый запрос — cache MISS
curl -s -D- http://localhost:8080/api/v1/services | grep X-Cache
# X-Cache: MISS

# Повторный запрос — cache HIT
curl -s -D- http://localhost:8080/api/v1/services | grep X-Cache
# X-Cache: HIT

# Проверка Redis
docker-compose exec redis redis-cli GET services:all
```

### Проверка rate limiting

```bash
# Отправить 11 запросов на логин — 11-й получит 429
for i in $(seq 1 11); do
  echo "Request $i:"
  curl -s -o /dev/null -w "%{http_code}" \
    -X POST http://localhost:8080/api/v1/auth/login \
    -H "Content-Type: application/json" \
    -d '{"login":"test","password":"test123"}'
  echo
done

# Проверить заголовки
curl -s -D- -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"ivan.petrov","password":"password123"}' | grep X-RateLimit
```

### Метрики

| Метрика | Способ измерения |
|---------|-----------------|
| Cache hit rate | `redis-cli INFO stats` → `keyspace_hits / (hits + misses)` |
| Cache memory | `redis-cli INFO memory` → `used_memory_human` |
| Rate limit rejections | Подсчёт HTTP 429 в логах nginx |
| Latency improvement | Сравнение `X-Cache: HIT` vs `MISS` response times |