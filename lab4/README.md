# Лабораторная работа: MongoDB

## Цель работы
Получить практические навыки работы с MongoDB, проектирования документной модели данных, выполнения CRUD операций и валидации схем.

## Задание

Для своего варианта задания выполните следующие задачи:

### 1. Проектирование документной модели
- Изучите выбранный вариант задания
- Спроектируйте структуру коллекций MongoDB для вашей системы
- Определите, какие сущности будут храниться в отдельных коллекциях
- Определите структуру документов для каждой коллекции
- Решите, где использовать embedded documents, а где references
- Обоснуйте выбор между embedded и references для каждой связи

### 2. Создание базы данных и коллекций
- Создайте MongoDB базу данных (использовать Docker)
- Создайте все необходимые коллекции
- Добавьте тестовые данные (минимум 10 документов в каждой коллекции)
- Используйте различные типы данных MongoDB (String, Number, Date, Array, Object и т.д.)

### 3. Реализация CRUD операций
- Напишите MongoDB запросы (или код на выбранном языке) для всех операций из вашего варианта задания:
  - Create (вставка документов)
  - Read (поиск документов с различными условиями)
  - Update (обновление документов)
  - Delete (удаление документов)
- Используйте различные операторы: `$eq`, `$ne`, `$gt`, `$lt`, `$in`, `$and`, `$or` и т.д.
- При необходимости используйте операции с массивами: `$push`, `$pull`, `$addToSet`

### 4. Валидация схем
- Создайте валидацию схем для минимум одной коллекции используя `$jsonSchema`
- Определите обязательные поля (required)
- Определите типы данных для полей
- Определите дополнительные ограничения (например, min/max для чисел, pattern для строк)
- Протестируйте валидацию, попытавшись вставить невалидные данные

### 5. Агрегации (опционально)
- Создайте один aggregation pipeline для сложного запроса
- Используйте стадии: `$match`, `$group`, `$project`, `$sort` и т.д.

### 6. Подключение базы данных MongoDB к API
- Реализуйте API для одной или нескольких сущностей из второго домашнего задания для работы с MongoDB

## Результат

Результат должен быть оформлен в виде следующих файлов, размещенных в вашем GitHub репозитории:

- `schema_design.md` – описание проектирования документной модели с обоснованием выбора embedded/references
- `data.js` или `data.json` – скрипт или файл с тестовыми данными
- `queries.js` или `queries.md` – MongoDB запросы для всех операций
- `validation.js` – скрипт с валидацией схем
- `README.md` – описание проекта, инструкции по запуску MongoDB и выполнению запросов
- Dockerfile и docker-compose.yml для запуска API и MongoDB (расширьте предыдущую работу)

### Критерии оценки
- Корректность проектирования документной модели
- Обоснованность выбора между embedded и references
- Качество MongoDB запросов
- Правильность валидации схем
- Качество документации

---

Продолжение [lab2](../lab2) — маркетплейс профессиональных услуг «Profi».
Гибридное хранилище: пользовательские данные и заказы остаются в **PostgreSQL** (как в lab3), каталог услуг перенесён в **MongoDB**.

## Архитектура

```
nginx:8080  ──┬──▶  user-service:8081    /api/v1/auth, /api/v1/users    ──▶  postgres:5432
              ├──▶  catalog-service:8082  /api/v1/services               ──▶  mongo:27017
              └──▶  order-service:8083    /api/v1/orders                 ──▶  postgres:5432
```

- **user-service** и **order-service** работают с PostgreSQL (пользовательские данные, заказы, связи M:N)
- **catalog-service** работает с MongoDB (каталог услуг — гибкая документная модель)

## Модель данных

### PostgreSQL (users, orders, order_services)

| Таблица | Ключевые колонки | Тип PK |
|---------|-----------------|--------|
| `users` | `id`, `login`, `email`, `first_name`, `last_name`, `password_hash` | BIGSERIAL |
| `orders` | `id`, `client_id` (FK → users), `status` | BIGSERIAL |
| `order_services` | `order_id` (FK → orders), `service_id` (TEXT — MongoDB ObjectId) | Composite PK |

### MongoDB (services)

| Коллекция | Ключевые поля | Типы MongoDB |
|-----------|--------------|--------------|
| `services` | `_id`, `title`, `description`, `price`, `provider_id`, `created_at` | ObjectId, String, Double, Date |

`provider_id` — строка с PostgreSQL `users.id`. `order_services.service_id` — строка с MongoDB `services._id`.

Дизайн и обоснование — в [schema_design.md](schema_design.md).

## Запуск

### Docker Compose (рекомендуется)

```bash
cd lab4
docker compose up --build
```

После запуска:
- PostgreSQL автоматически применяет `schema.sql` и `data.sql`
- Swagger UI: http://localhost:8081/swagger
- nginx (все сервисы): http://localhost:8080

### Загрузить тестовые данные MongoDB

```bash
docker compose exec mongo mongosh \
  "mongodb://profi:profi@localhost:27017/profidb?authSource=admin" \
  /dev/stdin < data.js
```

### Применить валидацию схем

```bash
docker compose exec mongo mongosh \
  "mongodb://profi:profi@localhost:27017/profidb?authSource=admin" \
  /dev/stdin < validation.js
```

### Запустить примеры запросов

```bash
docker compose exec mongo mongosh \
  "mongodb://profi:profi@localhost:27017/profidb?authSource=admin" \
  /dev/stdin < queries.js
```

### Подключиться к базам напрямую

```bash
# PostgreSQL
docker compose exec postgres psql -U profi -d profidb

# MongoDB
docker compose exec mongo mongosh \
  "mongodb://profi:profi@localhost:27017/profidb?authSource=admin"
```

## Переменные окружения

| Переменная | Значение в Docker | Используется |
|------------|-------------------|--------------|
| `dbconnstring` | `host=postgres port=5432 dbname=profidb user=profi password=profi` | user-service, order-service |
| `mongodbconnstring` | `mongodb://profi:profi@mongo:27017/profidb?authSource=admin` | catalog-service |
