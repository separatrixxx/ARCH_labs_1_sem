# Каталог событий (Event Catalog)

**Система:** Profi — маркетплейс профессиональных услуг

Здесь описаны все события, которые ходят через RabbitMQ в нашей системе. Для каждого события — что в payload, кто генерирует, кто обрабатывает.

---

## user.registered

Срабатывает при регистрации нового пользователя.

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

Что делают consumer-ы: notification-service шлёт welcome-email на указанный email, analytics-service записывает факт регистрации в метрики.

---

## service.created

Провайдер добавил новую услугу в каталог.

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
| price | float | Цена |
| provider_id | string | ID провайдера (users.id в PostgreSQL) |

При получении этого события cache-invalidation делает `DEL services:all` в Redis — чтобы при следующем GET-запросе список услуг подтянулся заново из MongoDB уже с новой услугой. Analytics просто логирует создание.

---

## order.created

Клиент оформил новый заказ (пока пустой, без услуг).

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
| status | string | Начальный статус (всегда pending) |

Notification-service уведомляет клиента, что заказ создан. Analytics фиксирует.

---

## order.service_added

В уже существующий заказ добавили конкретную услугу.

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

Notification-service уведомляет провайдера этой услуги — мол, на вас поступил заказ. Analytics считает статистику.

---

## order.status_changed

Статус заказа поменялся — подтвердили, завершили или отменили.

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

Допустимые статусы: `pending`, `confirmed`, `completed`, `cancelled`.

Это единственное событие, на которое подписаны сразу все три consumer-а: notification шлёт клиенту уведомление, cache-invalidation сбрасывает кеш заказов, analytics записывает переход между статусами.

---

## Сводная таблица

| Событие | Producer | Consumers | Exchange | Очереди |
|---------|----------|-----------|----------|---------|
| `user.registered` | user-service | notification, analytics | `profi.events` | notifications, analytics |
| `service.created` | catalog-service | cache, analytics | `profi.events` | cache_invalidation, analytics |
| `order.created` | order-service | notification, analytics | `profi.events` | notifications, analytics |
| `order.service_added` | order-service | notification, analytics | `profi.events` | notifications, analytics |
| `order.status_changed` | order-service | notification, cache, analytics | `profi.events` | notifications, cache_invalidation, analytics |

## Формат обёртки

Все события заворачиваются в одинаковую обёртку перед отправкой:

```json
{
    "event_id": "UUID v4",
    "event_type": "routing.key",
    "timestamp": "ISO 8601 UTC",
    "payload": { ... }
}
```

`event_id` нужен для идемпотентной обработки — если consumer получит одно и то же сообщение дважды (а при at-least-once это вполне возможно), он проверит ID и пропустит дубль. Все сообщения persistent (`delivery_mode=2`), все очереди durable — система переживает перезапуск брокера без потери данных.
