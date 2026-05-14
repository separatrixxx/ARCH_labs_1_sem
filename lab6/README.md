# Лабораторная работа 6: Проектирование Event-Driven архитектуры

## Цель работы

Получить навыки проектирования событийно-ориентированной архитектуры, работы с брокерами сообщений и применения паттерна CORS.

## Задание

Для своего варианта задания выполните следующие задачи:

### 1. Анализ событий в системе

– Изучите выбранный вариант задания  
– Определите события (events), которые происходят в вашей системе  
– Определите команды (commands), которые инициируют события  
– Определите, какие сервисы должны быть уведомлены о каждом событии

### 2. Проектирование Event-Driven архитектуры

– Определите компоненты системы, которые будут производителями событий (event producers)  
– Определите компоненты, которые будут потребителями событий (event consumers)  
– Определите типы событий и их структуру (payload)  
– Опишите поток событий в системе

### 3. Проектирование взаимодействия через брокер сообщений

– Выберите RabbitMQ или Apache Kafka  
– Определите формат сообщений для событий  
– Опишите гарантии доставки сообщений (at-least-once, exactly-once)

### 4. Применение паттерна CORS

– Определите, можно ли применить CORS в вашей системе  
– Если да, разделите операции на команды (write) и запросы (read)  
– Опишите, как события синхронизируют read и write модели

### 5. Реализация простого Event-Driven сервиса

– Настройте RabbitMQ/Kafka (использовать Docker)  
– Реализуйте простой producer, который публикует события  
– Реализуйте простой consumer, который обрабатывает события  
– Протестируйте взаимодействие

### 6. Документация событий

– Создайте каталог событий (event catalog) с описанием всех событий  
– Для каждого события укажите:  
  – Название события  
  – Структуру payload  
  – Производителя события  
  – Потребителей события  
  – Гарантии доставки

## Результат

Результат должен быть оформлен в виде следующих файлов, размещенных в вашем GitHub репозитории:

– `event_driven_design.md` – описание Event-Driven архитектуры  
– `event_catalog.md` – каталог событий с описанием  
– Исходный код с реализацией producer/consumer  
– `docker-compose.yml` – для запуска RabbitMQ/Kafka  
– `README.md` – описание проекта и инструкции по запуску

## Критерии оценки

– Корректность описания событий и команд  
– Качество проектирования Event-Driven архитектуры  
– Правильность выбора типов и роутинга  
– Применение паттерна CQRS (если применимо)  
– Качество каталога событий  
– Работоспособность реализации (если реализовано)

---

## Реализация: Profi — маркетплейс услуг (вариант 4)

**Студент:** Лохматов Никита, М8О-107СВ-25

### Архитектура

Три микросервиса-producer-а кидают события в RabbitMQ, оттуда они разлетаются по трём очередям - каждая для своего consumer-а:

```text
     ┌──────────────┐  ┌───────────────┐  ┌──────────────┐
     │ user-service  │  │catalog-service│  │ order-service │
     │  (producer)   │  │  (producer)   │  │  (producer)   │
     └──────┬───────┘  └──────┬────────┘  └──────┬────────┘
            │                 │                   │
            │    publish      │    publish         │    publish
            ▼                 ▼                   ▼
     ┌─────────────────────────────────────────────────────┐
     │                   RabbitMQ                           │
     │              profi.events (topic)                    │
     │                                                      │
     │  ┌──────────────┐ ┌──────────┐ ┌──────────────────┐ │
     │  │notifications │ │analytics │ │cache_invalidation│ │
     │  │    queue      │ │  queue   │ │     queue        │ │
     │  └──────┬───────┘ └────┬─────┘ └───────┬──────────┘ │
     └─────────┼──────────────┼───────────────┼────────────┘
               ▼              ▼               ▼
     ┌────────────────┐ ┌──────────┐ ┌────────────────────┐
     │  notification   │ │analytics │ │ cache-invalidation │
     │   consumer      │ │ consumer │ │     consumer       │
     └────────────────┘ └──────────┘ └────────────────────┘
```

### Почему RabbitMQ

Взял RabbitMQ с topic exchange - для маркетплейса с умеренной нагрузкой Kafka была бы избыточна. Topic exchange удобен тем, что разные consumer-ы подписываются на разные routing keys, а analytics может подписаться на `#` и ловить вообще всё. Подробное обоснование в `event_driven_design.md`.

### События системы

| Событие | Producer | Consumers |
|---------|----------|-----------|
| `user.registered` | user-service | notifications, analytics |
| `service.created` | catalog-service | cache_invalidation, analytics |
| `order.created` | order-service | notifications, analytics |
| `order.service_added` | order-service | notifications, analytics |
| `order.status_changed` | order-service | notifications, cache_invalidation, analytics |

Подробнее про payload и реакции consumer-ов — в `event_catalog.md`.

### CQRS

Разделение уже было в lab5, тут оно формализовано через события. Когда данные меняются, сервис публикует событие, cache-consumer подхватывает и инвалидирует нужные ключи в Redis. Подробнее в `event_driven_design.md`, раздел 4.

### Гарантии доставки

At-least-once: persistent messages, durable queues, manual ack, prefetch=1. Дубликаты отсекаются по `event_id` (UUID). Exactly-once не стали делать — слишком сложно для текущих требований, а at-least-once + идемпотентность дают тот же эффект.

### Структура файлов

```text
lab6/
├── README.md
├── event_driven_design.md
├── event_catalog.md
├── docker-compose.yaml
├── producer/
│   ├── Dockerfile
│   ├── requirements.txt
│   └── producer.py
└── consumer/
    ├── Dockerfile
    ├── requirements.txt
    └── consumer.py
```

### Запуск

```bash
cd lab6
docker-compose up --build
```

После запуска будет доступно:

- RabbitMQ AMQP: `localhost:5672`
- RabbitMQ Management UI: `http://localhost:15672` (логин/пароль: profi/profi)
- Producer публикует 6 событий, ждёт 30 секунд, повторяет
- Три consumer-а работают параллельно, каждый вычитывает свою очередь

### Проверка работы

Смотрим логи producer-а — он выводит каждое опубликованное событие:

```bash
docker-compose logs -f producer
```

```text
[PUBLISHED] user.registered: {"user_id": "13", "login": "test.user", ...}
[PUBLISHED] service.created: {"service_id": "660...", "title": "Дизайн логотипа", ...}
[PUBLISHED] order.created: {"order_id": "101", "client_id": "14", ...}
```

А в логах consumer-ов видно, как они обрабатывают события:

```bash
docker-compose logs -f notification-consumer
```

```text
[NOTIFICATIONS] Received: user.registered at 2024-04-15T10:30:00Z
  -> Отправить welcome-email на test.user@example.com для Тест Пользователь
[NOTIFICATIONS] Received: order.created at 2024-04-15T10:30:03Z
  -> Уведомить клиента 14: заказ #101 создан
```

```bash
docker-compose logs -f analytics-consumer
```

```text
[ANALYTICS] Received: user.registered at 2024-04-15T10:30:00Z
  -> Записать в аналитику: user.registered | {"user_id": "13", ...}
```

### Мониторинг через RabbitMQ UI

Открываем `http://localhost:15672`, логинимся (profi/profi). На вкладке Exchanges будет виден `profi.events` (topic), на вкладке Queues - три наши очереди с bindings и счётчиками сообщений. Можно руками кинуть тестовое сообщение через интерфейс.
