# Лабораторная работа 3: Проектирование и оптимизация реляционной базы данных

## Задание

Для своего варианта выполните следующие задачи:

### 1. Проектирование схемы базы данных

- Изучите выбранный вариант задания
- Спроектируйте схему реляционной БД для вашей системы
- Определите таблицы для всех сущностей из вашего варианта
- Определите первичные ключи (PK) для каждой таблицы
- Определите внешние ключи (FK) для связей между таблицами
- Определите типы данных для всех колонок
- Определите ограничения (constraints): NOT NULL, UNIQUE, CHECK

### 2. Создание базы данных

- Создайте PostgreSQL базу данных (можно использовать Docker)
- Создайте все таблицы с помощью SQL CREATE TABLE
- Добавьте тестовые данные (минимум 10 записей в каждой таблице)

### 3. Создание индексов

- Проанализируйте, какие запросы будут выполняться часто
- Создайте индексы для:
    - Первичных ключей (автоматически создаются)
    - Внешних ключей
    - Колонок, используемых в WHERE условиях
    - Колонок, используемых в JOIN
- Объясните, для чего нужен каждый индекс

### 4. Оптимизация запросов

- Напишите SQL запросы для всех операций из вашего варианта задания
- Используйте EXPLAIN для анализа плана выполнения запросов
- Оптимизируйте запросы (добавьте индексы, перепишите запрос)
- Сравните планы выполнения до и после оптимизации

### 5. Партиционирование (опционально)

- Если в вашей системе есть таблицы с большим количеством данных, которые можно разделить по времени или другому критерию, спроектируйте партиционирование
- Опишите стратегию партиционирования

### 6. Подключение API к базе данных

- Подключите API, реализованные в прошлой домашней работе к созданной базе данных

---

Продолжение [lab2](../lab2) — маркетплейс профессиональных услуг «Profi». In-memory хранилище заменено на PostgreSQL. Все три сервиса (user, catalog, order) подключены к единой БД.

## Архитектура

```
nginx:8080  ──┬──▶  user-service:8081    /api/v1/auth, /api/v1/users
              ├──▶  catalog-service:8082  /api/v1/services
              └──▶  order-service:8083    /api/v1/orders
                         │
                         ▼
                   postgres:5432
                   (profidb)
```

---

## Схема базы данных

```
users
  id            BIGSERIAL      PK
  login         VARCHAR(64)    NOT NULL UNIQUE
  email         VARCHAR(256)   NOT NULL UNIQUE
  first_name    VARCHAR(128)   NOT NULL
  last_name     VARCHAR(128)   NOT NULL
  password_hash VARCHAR(256)   NOT NULL
  created_at    TIMESTAMPTZ    NOT NULL DEFAULT now()

services
  id            BIGSERIAL      PK
  title         VARCHAR(256)   NOT NULL
  description   TEXT
  price         FLOAT8         NOT NULL CHECK (price >= 0)
  provider_id   BIGINT         NOT NULL FK → users(id)
  created_at    TIMESTAMPTZ    NOT NULL DEFAULT now()

orders
  id            BIGSERIAL      PK
  client_id     BIGINT         NOT NULL FK → users(id)
  status        VARCHAR(32)    NOT NULL DEFAULT 'pending'
                               CHECK (status IN ('pending','confirmed','completed','cancelled'))
  created_at    TIMESTAMPTZ    NOT NULL DEFAULT now()

order_services  (M:N)
  order_id      BIGINT         NOT NULL FK → orders(id)
  service_id    BIGINT         NOT NULL FK → services(id)
  PRIMARY KEY (order_id, service_id)
```

### Связи

- `services.provider_id → users.id` — пользователь предоставляет услуги
- `orders.client_id → users.id` — пользователь создаёт заказы
- `order_services` — M:N между заказами и услугами

### Индексы

| Индекс | Колонки | Назначение |
|--------|---------|------------|
| `users_login_key` (UNIQUE, авто) | `login` | Логин при аутентификации |
| `idx_users_name` | `lower(first_name), lower(last_name)` | Поиск по маске имени |
| `idx_services_provider_id` | `provider_id` | Услуги конкретного провайдера |
| `idx_orders_client_id` | `client_id` | Все заказы клиента |
| `idx_orders_status` | `status` | Фильтрация по статусу |
| `idx_orders_client_status` | `client_id, status` | Частый комбинированный запрос |
| `idx_order_services_service_id` | `service_id` | Обратная навигация по услуге |

---

## Запуск

### Docker Compose

```bash
docker compose up --build
```

При первом старте PostgreSQL автоматически применяет `schema.sql` и `data.sql` через `/docker-entrypoint-initdb.d/`.

Доступно после запуска:
- Swagger UI: http://localhost:8081/swagger
- nginx (все сервисы): http://localhost:8080

### Пересоздать схему и данные

```bash
docker compose exec postgres psql -U profi -d profidb \
  -f /docker-entrypoint-initdb.d/01_schema.sql \
  -f /docker-entrypoint-initdb.d/02_data.sql
```

### Подключиться к базе напрямую

```bash
docker compose exec postgres psql -U profi -d profidb
```

### Локальная сборка

```bash
# Запустить только postgres
docker compose up -d postgres

# Собрать
cmake -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DFETCHCONTENT_UPDATES_DISCONNECTED=ON \
      -DUSERVER_FEATURE_POSTGRESQL=ON \
      -DUSE_POSTGRESQL=ON \
      -DPostgreSQLInternal_ROOT=/usr/lib/postgresql/14
cmake --build build --target user_service catalog_service order_service

# Запустить каждый сервис (в отдельных терминалах)
./build/user_service    --config configs/user-service/static_config.yaml    --config_vars configs/config_vars.yaml
./build/catalog_service --config configs/catalog-service/static_config.yaml --config_vars configs/config_vars.yaml
./build/order_service   --config configs/order-service/static_config.yaml   --config_vars configs/config_vars.yaml
```

---

## Переменные окружения

| Переменная | Значение в Docker | Описание |
|------------|-------------------|----------|
| `PGCONNSTRING` | `host=postgres port=5432 dbname=profidb user=profi password=profi` | Строка подключения к PostgreSQL |
