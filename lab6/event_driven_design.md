# Проектирование Event-Driven архитектуры

**Система:** Profi — маркетплейс профессиональных услуг
**Вариант:** 4 (Система маркетплейса услуг)

---

## 1. Анализ событий в системе

### 1.1 События (Events)

| Событие | Описание | Источник |
|---------|----------|----------|
| `user.registered` | Новый пользователь зарегистрировался | user-service |
| `service.created` | В каталог добавлена новая услуга | catalog-service |
| `order.created` | Клиент создал новый заказ | order-service |
| `order.service_added` | В заказ добавлена услуга | order-service |
| `order.status_changed` | Статус заказа изменился | order-service |

### 1.2 Команды (Commands)

| Команда | Endpoint | Инициируемое событие |
|---------|----------|---------------------|
| Зарегистрировать пользователя | `POST /api/v1/auth/register` | `user.registered` |
| Создать услугу | `POST /api/v1/services` | `service.created` |
| Создать заказ | `POST /api/v1/orders` | `order.created` |
| Добавить услугу в заказ | `POST /api/v1/orders/{id}/services` | `order.service_added` |
| Изменить статус заказа | `PUT /api/v1/orders/{id}/status` | `order.status_changed` |

### 1.3 Маршрутизация событий к сервисам

| Событие | Кого уведомить | Зачем |
|---------|---------------|-------|
| `user.registered` | notification-service | Отправить welcome-email |
| `user.registered` | analytics-service | Статистика регистраций |
| `service.created` | cache-invalidation | Сбросить кеш `services:all` |
| `service.created` | analytics-service | Статистика каталога |
| `order.created` | notification-service | Уведомить клиента о создании заказа |
| `order.created` | analytics-service | Статистика заказов |
| `order.service_added` | notification-service | Уведомить провайдера о новом заказе |
| `order.status_changed` | notification-service | Уведомить клиента об изменении статуса |
| `order.status_changed` | cache-invalidation | Обновить кеш заказов |

---

## 2. Проектирование Event-Driven архитектуры

### 2.1 Producers (производители событий)

| Producer | Генерируемые события |
|----------|---------------------|
| user-service | `user.registered` |
| catalog-service | `service.created` |
| order-service | `order.created`, `order.service_added`, `order.status_changed` |

Каждый микросервис публикует событие после успешного выполнения команды. Событие содержит факт того, что произошло, а не команду к действию.

### 2.2 Consumers (потребители событий)

| Consumer | Подписка | Назначение |
|----------|----------|-----------|
| notification-service | `user.registered`, `order.created`, `order.service_added`, `order.status_changed` | Рассылка уведомлений (email, push) |
| analytics-service | `#` (все события) | Сбор аналитики и метрик |
| cache-invalidation | `service.created`, `order.status_changed` | Инвалидация Redis-кеша |

### 2.3 Типы событий и структура payload

Каждое событие представляет собой JSON-сообщение с единой оболочкой:

```json
{
    "event_id": "550e8400-e29b-41d4-a716-446655440000",
    "event_type": "user.registered",
    "timestamp": "2024-04-15T10:30:00Z",
    "payload": { ... }
}
```

Поля оболочки:
- `event_id` — UUID, уникальный идентификатор события (для идемпотентности)
- `event_type` — тип события (совпадает с routing key)
- `timestamp` — время возникновения (ISO 8601, UTC)
- `payload` — тело события, зависит от типа

### 2.4 Поток событий

```
┌──────────────┐    POST /register     ┌──────────────┐
│    Client     │ ──────────────────> │ user-service  │
└──────────────┘                      └──────┬───────┘
                                              │ publish
                                              ▼
                                     ┌────────────────┐
                                     │   RabbitMQ      │
                                     │ profi.events    │
                                     │ (topic exchange)│
                                     └──┬──────┬──────┘
                          routing key   │      │   routing key
                        user.registered │      │   #
                                        ▼      ▼
                              ┌───────────┐  ┌───────────┐
                              │notifications│ │ analytics │
                              │   queue    │  │   queue   │
                              └─────┬─────┘  └─────┬─────┘
                                    │               │
                              ┌─────▼──────┐ ┌─────▼──────┐
                              │notification│ │ analytics  │
                              │  consumer  │ │  consumer  │
                              └────────────┘ └────────────┘
                              Отправить email  Записать метрику
```

---

## 3. Взаимодействие через брокер сообщений

### 3.1 Выбор брокера: RabbitMQ

| Критерий | RabbitMQ | Apache Kafka |
|----------|----------|-------------|
| Модель | Push (брокер доставляет) | Pull (consumer запрашивает) |
| Routing | Гибкий (topic, direct, fanout) | Только partition-based |
| Порядок | В пределах очереди | В пределах partition |
| Масштаб | До тысяч msg/sec | Миллионы msg/sec |
| Сложность | Низкая | Высокая (ZooKeeper/KRaft) |

**Выбор: RabbitMQ.**

Обоснование:
- Маркетплейс услуг — средняя нагрузка (сотни событий/сек), Kafka избыточна
- Topic exchange RabbitMQ идеально подходит для маршрутизации событий по типу
- Простая настройка через Docker (один контейнер без ZooKeeper)
- Встроенный management UI для мониторинга
- Поддержка подтверждений доставки (publisher confirms + consumer ack)

### 3.2 Топология

**Exchange:** `profi.events` (тип: `topic`, durable)

**Очереди и привязки:**

| Очередь | Binding keys | Consumer |
|---------|-------------|----------|
| `notifications` | `user.registered`, `order.created`, `order.service_added`, `order.status_changed` | notification-service |
| `analytics` | `#` | analytics-service |
| `cache_invalidation` | `service.created`, `order.status_changed` | cache-invalidation-service |

### 3.3 Формат сообщений

- Content-Type: `application/json`
- Delivery mode: `persistent` (сообщения сохраняются на диск)
- Кодировка: UTF-8

Пример сообщения в очереди `notifications`:

```json
{
    "event_id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
    "event_type": "order.created",
    "timestamp": "2024-04-15T14:00:00Z",
    "payload": {
        "order_id": "101",
        "client_id": "14",
        "status": "pending"
    }
}
```

### 3.4 Гарантии доставки

Используется **at-least-once** delivery:

| Механизм | Сторона | Как работает |
|----------|---------|-------------|
| Publisher confirms | Producer | Брокер подтверждает получение сообщения |
| Persistent messages | Брокер | `delivery_mode=2` — сообщение записывается на диск |
| Durable queues | Брокер | Очереди переживают перезапуск RabbitMQ |
| Manual ack | Consumer | Consumer подтверждает обработку (`basic_ack`) |
| Prefetch=1 | Consumer | Одно сообщение за раз — нет потерь при падении |

Для обеспечения идемпотентности consumer-ы могут проверять `event_id` и игнорировать дубликаты.

---

## 4. Применение паттерна CQRS

### 4.1 Применимость к системе Profi

CQRS (Command Query Responsibility Segregation) применим к нашей системе, так как:
- **Чтение значительно преобладает** над записью (каталог услуг просматривают чаще, чем обновляют)
- Уже есть разделение: PostgreSQL/MongoDB для записи, Redis для чтения (реализовано в lab5)
- Разные требования к моделям: write-модель нормализована (3NF), read-модель денормализована (JSON в Redis)

### 4.2 Разделение на команды и запросы

**Команды (Write):**

| Команда | Endpoint | Write-модель |
|---------|----------|-------------|
| Регистрация | `POST /auth/register` | PostgreSQL `users` |
| Создание услуги | `POST /services` | MongoDB `services` |
| Создание заказа | `POST /orders` | PostgreSQL `orders` |
| Добавление услуги в заказ | `POST /orders/{id}/services` | PostgreSQL `order_services` |

**Запросы (Read):**

| Запрос | Endpoint | Read-модель |
|--------|----------|------------|
| Список услуг | `GET /services` | Redis `services:all` |
| Услуга по ID | `GET /services/{id}` | Redis `service:{id}` |
| Поиск пользователя | `GET /users?login=` | Redis `user:login:{login}` |
| Заказы клиента | `GET /orders` | PostgreSQL (без кеша — часто меняется) |

### 4.3 Синхронизация read и write моделей через события

```
Клиент
  │
  │ POST /api/v1/services
  ▼
┌─────────────────┐
│ catalog-service  │
│   (Command)      │
│                  │
│ 1. Записать в    │──────────────┐
│    MongoDB       │              │
│ 2. Опубликовать  │              │
│    событие       │              ▼
└─────────────────┘     ┌────────────────┐
                        │   RabbitMQ      │
                        │ service.created │
                        └───────┬────────┘
                                │
                                ▼
                     ┌─────────────────────┐
                     │ cache-invalidation   │
                     │   (Event Handler)    │
                     │                      │
                     │ DEL services:all     │
                     │ (Redis read-model)   │
                     └─────────────────────┘
                                │
                                │ Следующий GET /services
                                ▼
                     ┌─────────────────────┐
                     │ catalog-service      │
                     │   (Query)            │
                     │                      │
                     │ Cache MISS → MongoDB │
                     │ → SET services:all   │
                     │ → вернуть клиенту    │
                     └─────────────────────┘
```

Текущая реализация (lab5) использует синхронную инвалидацию: при POST сервис сам делает `DEL` в Redis. В полноценной CQRS-архитектуре это делается асинхронно через события, что обеспечивает слабую связанность сервисов.

### 4.4 Eventual Consistency

При асинхронной синхронизации через события возникает eventual consistency:
- После записи в write-модель read-модель обновляется с задержкой (обычно < 100 мс)
- Для каталога услуг это приемлемо: пользователь увидит новую услугу через несколько секунд
- Для критичных операций (авторизация, проверка баланса) используется прямое чтение из write-модели

---

## 5. Реализация

### 5.1 Компоненты

| Компонент | Технология | Назначение |
|-----------|-----------|-----------|
| Брокер | RabbitMQ 3 (management) | Topic exchange, durable queues |
| Producer | Python + pika | Публикация событий из микросервисов |
| notification-consumer | Python + pika | Обработка событий уведомлений |
| analytics-consumer | Python + pika | Сбор аналитики по всем событиям |
| cache-consumer | Python + pika | Инвалидация Redis-кеша |

### 5.2 Топология RabbitMQ

Producer создаёт топологию при запуске:
1. Объявляет exchange `profi.events` (topic, durable)
2. Объявляет 3 durable-очереди
3. Привязывает очереди к exchange по routing keys

### 5.3 Цикл работы

1. Producer публикует 6 событий с интервалом 1 секунда
2. RabbitMQ маршрутизирует события по routing keys в соответствующие очереди
3. Каждый consumer читает свою очередь и обрабатывает события
4. Consumer отправляет `basic_ack` после обработки

### 5.4 Управление UI

RabbitMQ Management доступен на `http://localhost:15672` (profi/profi):
- Мониторинг очередей и сообщений
- Просмотр bindings и exchanges
- Ручная публикация тестовых сообщений
