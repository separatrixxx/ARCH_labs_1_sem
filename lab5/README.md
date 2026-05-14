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

К lab4 добавился Redis — он работает как общий кеш и rate-limiter для всех трёх сервисов:

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

### Что нового по сравнению с lab4

- **Redis 7** — единый кеш и rate-limit слой
- **Кеширование (Cache-Aside)** — для трёх GET-эндпоинтов
- **Rate limiting (Fixed Window Counter)** — для login и register

Подробное обоснование выбора стратегий — в `performance_design.md`.

### Кеширование

Стратегия Cache-Aside: сначала смотрим в Redis, если нет — идём в БД и сохраняем результат в кеш.

| Endpoint | Ключ Redis | TTL | Инвалидация |
|----------|-----------|-----|-------------|
| `GET /api/v1/services` | `services:all` | 60 с | При `POST /api/v1/services` |
| `GET /api/v1/services/{id}` | `service:{id}` | 120 с | По TTL |
| `GET /api/v1/users?login=...` | `user:login:{login}` | 30 с | По TTL |

В ответе есть заголовок `X-Cache: HIT` или `X-Cache: MISS` — по нему видно, откуда пришли данные.

Если Redis упал — запросы идут напрямую к БД, сервис не ломается (fail-open).

### Rate limiting

Fixed Window Counter через атомарный INCR в Redis.

| Endpoint | Лимит | Окно |
|----------|-------|------|
| `POST /api/v1/auth/login` | 10 запросов | 60 сек |
| `POST /api/v1/auth/register` | 5 запросов | 60 сек |

При превышении — HTTP 429. В каждом ответе заголовки:
```
X-RateLimit-Limit: 10
X-RateLimit-Remaining: 7
X-RateLimit-Reset: 45
```

### Структура файлов

```
lab5/
├── README.md
├── performance_design.md
├── docker-compose.yaml
├── Dockerfile
├── Dockerfile.postgres
├── Dockerfile.nginx
├── CMakeLists.txt
├── schema.sql
├── data.sql
├── data.js
├── configs/
│   ├── config_vars.yaml
│   ├── nginx/nginx.conf
│   ├── user-service/static_config.yaml
│   ├── catalog-service/static_config.yaml
│   └── order-service/static_config.yaml
└── src/
    ├── shared/
    │   ├── redis_cache.hpp      # RedisCache компонент (заголовок)
    │   └── redis_cache.cpp      # Get/Set/Del + CheckRateLimit
    ├── user-service/
    │   ├── main.cpp
    │   └── handlers/
    │       ├── auth_handler.hpp
    │       ├── auth_handler.cpp  # rate limiting для login/register
    │       ├── users_handler.hpp
    │       └── users_handler.cpp # кеш user:login:{login}
    ├── catalog-service/
    │   ├── main.cpp
    │   └── handlers/
    │       ├── services_handler.hpp
    │       └── services_handler.cpp # кеш services:all + инвалидация
    └── order-service/
        └── main.cpp
```

### Запуск

```bash
cd lab5
docker-compose up --build
```

После запуска:
- API через nginx: http://localhost:8080
- Swagger: http://localhost:8080/swagger
- Redis CLI: `docker-compose exec redis redis-cli`

### Проверка кеширования

```bash
# Первый запрос — MISS, данные из БД
curl -s -D- http://localhost:8080/api/v1/services | grep X-Cache
# X-Cache: MISS

# Второй запрос — HIT, данные из Redis
curl -s -D- http://localhost:8080/api/v1/services | grep X-Cache
# X-Cache: HIT

# Заглянуть в Redis напрямую
docker-compose exec redis redis-cli GET services:all
```

### Проверка rate limiting

```bash
# Шлём 11 запросов подряд — на 11-м получим 429
for i in $(seq 1 11); do
  echo "Request $i:"
  curl -s -o /dev/null -w "%{http_code}" \
    -X POST http://localhost:8080/api/v1/auth/login \
    -H "Content-Type: application/json" \
    -d '{"login":"test","password":"test123"}'
  echo
done

# Посмотреть заголовки с лимитами
curl -s -D- -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"ivan.petrov","password":"password123"}' | grep X-RateLimit
```

### Метрики

| Метрика | Как посмотреть |
|---------|---------------|
| Cache hit rate | `redis-cli INFO stats` → `keyspace_hits / (hits + misses)` |
| Память Redis | `redis-cli INFO memory` → `used_memory_human` |
| 429-е ответы | Считаем в логах nginx |
| Разница в latency | Сравнить время ответа при HIT и MISS |
