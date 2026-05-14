# Лабораторная работа 4: MongoDB

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

Это продолжение [lab2](../lab2) — маркетплейс «Profi».

Основная идея lab4: перевести каталог услуг на MongoDB, оставив пользователей и заказы в PostgreSQL. Получается гибридное хранилище — реляционка для структурированных данных со связями, документная БД для каталога с гибкой схемой.

## Архитектура

```
nginx:8080  ──┬──▶  user-service:8081    /api/v1/auth, /api/v1/users    ──▶  postgres:5432
              ├──▶  catalog-service:8082  /api/v1/services               ──▶  mongo:27017
              └──▶  order-service:8083    /api/v1/orders                 ──▶  postgres:5432
```

user-service и order-service по-прежнему работают с PostgreSQL, а catalog-service теперь ходит в MongoDB. Почему именно каталог вынесен в Mongo — описано в [schema_design.md](schema_design.md), если коротко: у услуг гибкая структура, которую удобнее хранить в документах.

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

`provider_id` — строковое представление PostgreSQL `users.id`. А `order_services.service_id` — строка с MongoDB ObjectId. Такая cross-database связь, целостность на уровне приложения.

Подробнее про дизайн и обоснование — в [schema_design.md](schema_design.md).

## Запуск

### Docker Compose

```bash
cd lab4
docker compose up --build
```

PostgreSQL сам подтянет `schema.sql` и `data.sql` при первом запуске. После этого:
- Swagger UI: http://localhost:8081/swagger
- Все сервисы через nginx: http://localhost:8080

### Загрузить тестовые данные в MongoDB

MongoDB данные нужно залить руками, потому что там нет init-скриптов как в PostgreSQL:

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
