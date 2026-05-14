# Проектирование гибридной модели данных — Profi Services

## Система: маркетплейс профессиональных услуг

Три основные сущности: **пользователи**, **услуги**, **заказы**.

В lab3 всё лежало в PostgreSQL, но тут решил попробовать гибридный подход — пользователей и заказы оставить в реляционке, а каталог услуг вынести в MongoDB. Ниже объясню, почему так.

---

## Обоснование разделения по БД

| Сущность | Хранилище | Причина |
|----------|-----------|---------|
| `users` | PostgreSQL | Персональные данные, строгие ограничения (UNIQUE login/email), ACID-транзакции при регистрации/логине |
| `orders` + `order_services` | PostgreSQL | Реляционные связи с пользователями (FK → users), транзакционная целостность статусов |
| `services` | MongoDB | Каталог с гибкой структурой: описания произвольной длины, возможность добавления атрибутов без миграции |

По сути, логика такая: если данные жёстко структурированы и связаны между собой через FK — PostgreSQL. Если структура может меняться и расширяться (теги, категории, произвольные атрибуты у разных услуг) — MongoDB. Заказы привязаны к пользователям через FK, поэтому им место в реляционке. А вот услуги — это по сути каталожные карточки, у которых со временем могут появляться новые поля, и ALTER TABLE каждый раз делать не хочется.

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

Тут есть нюанс: `service_id` — TEXT, а не числовой тип, потому что на другой стороне MongoDB, и там ID — это ObjectId (24-символьная hex-строка типа `660000000000000000000001`). FK-ограничение между PostgreSQL и MongoDB поставить нельзя, так что целостность проверяется на уровне приложения: catalog-service при добавлении услуги в заказ сначала проверяет, что такая услуга вообще существует.

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

Пара моментов по полям:

- `provider_id` — это cross-database reference, по сути строковое представление `users.id` из PostgreSQL (т.е. "1", "3", "11", а не ObjectId). Не очень красиво, но работает — при запросе catalog-service может сходить в user-service и проверить, существует ли такой провайдер.
- `description` — свободный текст без ограничений на длину. В PostgreSQL пришлось бы выбирать VARCHAR(N) или TEXT, тут же документная модель не парится по этому поводу.
- Главное преимущество — если завтра понадобятся теги, категории или вложенные атрибуты, добавляем их прямо в документ без ALTER TABLE и миграций.

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
| `users_login_key` (UNIQUE) | `login` | Аутентификация — ищем пользователя по логину |
| `idx_users_name` | `lower(first_name), lower(last_name)` | Поиск по имени (case-insensitive) |
| `idx_orders_client_id` | `client_id` | Получить все заказы клиента |
| `idx_orders_status` | `status` | Фильтрация заказов по статусу |
| `idx_orders_client_status` | `client_id, status` | Составной — «все pending-заказы клиента X» |

### MongoDB

```js
db.services.createIndex({ provider_id: 1 })
db.services.createIndex({ price: 1 })
```

- `provider_id` — чтобы быстро находить все услуги конкретного провайдера
- `price` — для сортировки и фильтрации каталога по цене
