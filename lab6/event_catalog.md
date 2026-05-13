# Каталог событий (Event Catalog)

**Система:** Profi — маркетплейс профессиональных услуг

---

## user.registered

Новый пользователь зарегистрирован в системе.

**Routing key:** `user.registered`
**Producer:** user-service (`POST /api/v1/auth/register`)
**Consumers:** notification-service, analytics-service
**Гарантия доставки:** at-least-once

**Payload:**

```json
{
    "user_id": "13",
    "login": "ivan.petrov",
    "email": "ivan.petrov@example.com",
    "first_name": "Иван",
    "last_name": "Петров"
}
```

| Поле | Тип | Описание |
|------|-----|----------|
| user_id | string | ID пользователя в PostgreSQL |
| login | string | Логин |
| email | string | Email для отправки уведомлений |
| first_name | string | Имя |
| last_name | string | Фамилия |

**Реакции:**
- notification-service: отправить welcome-email
- analytics-service: записать регистрацию в метрики

---

## service.created

В каталог добавлена новая услуга.

**Routing key:** `service.created`
**Producer:** catalog-service (`POST /api/v1/services`)
**Consumers:** cache-invalidation-service, analytics-service
**Гарантия доставки:** at-least-once

**Payload:**

```json
{
    "service_id": "660000000000000000000101",
    "title": "Дизайн логотипа",
    "price": 5000.0,
    "provider_id": "13"
}
```

| Поле | Тип | Описание |
|------|-----|----------|
| service_id | string | MongoDB ObjectId услуги |
| title | string | Название услуги |
| price | float | Цена услуги |
| provider_id | string | ID провайдера (users.id в PostgreSQL) |

**Реакции:**
- cache-invalidation: `DEL services:all` в Redis
- analytics-service: записать создание услуги

---

## order.created

Клиент создал новый заказ.

**Routing key:** `order.created`
**Producer:** order-service (`POST /api/v1/orders`)
**Consumers:** notification-service, analytics-service
**Гарантия доставки:** at-least-once

**Payload:**

```json
{
    "order_id": "101",
    "client_id": "14",
    "status": "pending"
}
```

| Поле | Тип | Описание |
|------|-----|----------|
| order_id | string | ID заказа в PostgreSQL |
| client_id | string | ID клиента |
| status | string | Начальный статус заказа |

**Реакции:**
- notification-service: уведомить клиента о создании заказа
- analytics-service: записать создание заказа

---

## order.service_added

В существующий заказ добавлена услуга.

**Routing key:** `order.service_added`
**Producer:** order-service (`POST /api/v1/orders/{id}/services`)
**Consumers:** notification-service, analytics-service
**Гарантия доставки:** at-least-once

**Payload:**

```json
{
    "order_id": "101",
    "service_id": "660000000000000000000101"
}
```

| Поле | Тип | Описание |
|------|-----|----------|
| order_id | string | ID заказа |
| service_id | string | MongoDB ObjectId добавленной услуги |

**Реакции:**
- notification-service: уведомить провайдера услуги о заказе
- analytics-service: записать добавление услуги

---

## order.status_changed

Статус заказа изменился.

**Routing key:** `order.status_changed`
**Producer:** order-service (`PUT /api/v1/orders/{id}/status`)
**Consumers:** notification-service, cache-invalidation-service, analytics-service
**Гарантия доставки:** at-least-once

**Payload:**

```json
{
    "order_id": "101",
    "old_status": "pending",
    "new_status": "confirmed"
}
```

| Поле | Тип | Описание |
|------|-----|----------|
| order_id | string | ID заказа |
| old_status | string | Предыдущий статус |
| new_status | string | Новый статус |

Допустимые значения статуса: `pending`, `confirmed`, `completed`, `cancelled`.

**Реакции:**
- notification-service: уведомить клиента об изменении статуса
- cache-invalidation: сбросить кеш заказов клиента
- analytics-service: записать переход статуса

---

## Сводная таблица

| Событие | Producer | Consumers | Exchange | Очереди |
|---------|----------|-----------|----------|---------|
| `user.registered` | user-service | notification, analytics | `profi.events` | notifications, analytics |
| `service.created` | catalog-service | cache, analytics | `profi.events` | cache_invalidation, analytics |
| `order.created` | order-service | notification, analytics | `profi.events` | notifications, analytics |
| `order.service_added` | order-service | notification, analytics | `profi.events` | notifications, analytics |
| `order.status_changed` | order-service | notification, cache, analytics | `profi.events` | notifications, cache_invalidation, analytics |

## Формат оболочки события

Все события заворачиваются в единую оболочку:

```json
{
    "event_id": "UUID v4",
    "event_type": "routing.key",
    "timestamp": "ISO 8601 UTC",
    "payload": { ... }
}
```

- `event_id` используется для идемпотентной обработки (consumer отбрасывает дубликаты)
- `delivery_mode=2` (persistent) для всех сообщений
- Все очереди durable — переживают перезапуск брокера
