# Проектирование гибридной модели данных — Profi Services

## Система: маркетплейс профессиональных услуг

Три сущности: **пользователи**, **услуги**, **заказы**.

В отличие от lab3 (всё в PostgreSQL), здесь используется **гибридный подход**: пользовательские данные и заказы хранятся в реляционной БД (PostgreSQL), а каталог услуг — в документной БД (MongoDB).

---

## Обоснование разделения по БД

| Сущность | Хранилище | Причина |
|----------|-----------|---------|
| `users` | PostgreSQL | Персональные данные, строгие ограничения (UNIQUE login/email), ACID-транзакции при регистрации/логине |
| `orders` + `order_services` | PostgreSQL | Реляционные связи с пользователями (FK → users), транзакционная целостность статусов |
| `services` | MongoDB | Каталог с гибкой структурой: описания произвольной длины, возможность добавления произвольных атрибутов (теги, категории) без миграции схемы |

---

## PostgreSQL: таблицы

### `users`

| Колонка | Тип | Ограничения |
|---------|-----|-------------|
| `id` | BIGSERIAL | PK |
| `login` | VARCHAR(64) | NOT NULL, UNIQUE |
| `email` | VARCHAR(256) | NOT NULL, UNIQUE |
| `first_name` | VARCHAR(128) | NOT NULL |
| `last_name` | VARCHAR(128) | NOT NULL |
| `password_hash` | VARCHAR(256) | NOT NULL |
| `created_at` | TIMESTAMPTZ | NOT NULL DEFAULT now() |

### `orders`

| Колонка | Тип | Ограничения |
|---------|-----|-------------|
| `id` | BIGSERIAL | PK |
| `client_id` | BIGINT | NOT NULL, FK → users(id) |
| `status` | VARCHAR(32) | NOT NULL, CHECK IN ('pending','confirmed','completed','cancelled') |
| `created_at` | TIMESTAMPTZ | NOT NULL DEFAULT now() |

### `order_services` (M:N)

| Колонка | Тип | Ограничения |
|---------|-----|-------------|
| `order_id` | BIGINT | NOT NULL, FK → orders(id) |
| `service_id` | TEXT | NOT NULL — хранит MongoDB ObjectId услуги |
| PK | | (order_id, service_id) |

`service_id` — TEXT, а не BIGINT, потому что каталог услуг хранится в MongoDB и идентифицируется ObjectId (24-символьная hex-строка). FK-ограничение невозможно через границу СУБД, поэтому консистентность обеспечивается на уровне приложения (catalog-service проверяет существование услуги перед добавлением в заказ).

---

## MongoDB: коллекция `services`

```json
{
  "_id":         ObjectId,
  "title":       String,
  "description": String,
  "price":       Double,
  "provider_id": String,
  "created_at":  Date
}
```

### Особенности

- `provider_id` — **cross-database reference**: строка с PostgreSQL `users.id`. Не ObjectId, а числовой ID в виде строки ("1", "3", "11").
- `description` — свободный текст, который может расширяться до объёмных описаний, списков, markdown. Документная модель не ограничивает длину.
- Возможность добавления вложенных полей (tags, attributes) без ALTER TABLE.

---

## Типы данных

### PostgreSQL

| Колонка | Тип PG |
|---------|--------|
| `id` | BIGSERIAL |
| `login`, `email` | VARCHAR |
| `status` | VARCHAR + CHECK |
| `created_at` | TIMESTAMPTZ |
| `service_id` | TEXT (ObjectId) |

### MongoDB

| Поле | Тип BSON | Пример |
|------|----------|--------|
| `_id` | ObjectId | `ObjectId("660000000000000000000001")` |
| `title`, `description` | String | `"Ремонт компьютеров"` |
| `price` | Double | `2500.0` |
| `provider_id` | String | `"1"` (PG users.id) |
| `created_at` | Date | `ISODate("2024-01-20")` |

---

## Индексы

### PostgreSQL

| Индекс | Колонки | Назначение |
|--------|---------|------------|
| `users_login_key` (UNIQUE) | `login` | Аутентификация |
| `idx_users_name` | `lower(first_name), lower(last_name)` | Поиск по имени |
| `idx_orders_client_id` | `client_id` | Заказы клиента |
| `idx_orders_status` | `status` | Фильтрация по статусу |
| `idx_orders_client_status` | `client_id, status` | Составной запрос |

### MongoDB

```js
db.services.createIndex({ provider_id: 1 })
db.services.createIndex({ price: 1 })
```

- `provider_id` — все услуги конкретного провайдера
- `price` — сортировка и фильтрация по цене
