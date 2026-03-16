# Лабораторная работа 2: Разработка REST API сервиса

## Задание

Выберите нужный вариант из файла `homework_variants.pdf` (варианты 1–24) и выполните следующие задачи:

### 1. Проектирование REST API

- Изучите выбранный вариант задания
- Спроектируйте REST API endpoints для всех операций из вашего варианта
- Используйте правильные HTTP методы (GET, POST, PUT, DELETE, PATCH)
- Используйте правильные HTTP статус-коды
- Спроектируйте структуру URL (ресурсы, вложенные ресурсы)
- Определите структуру Request/Response для каждого endpoint

### 2. Реализация REST API сервиса

- Реализуйте REST API сервис на выбранном языке и фреймворке (Python FastAPI, C++ Poco, Yandex Userver)
- Реализуйте минимум 5 API endpoints из вашего варианта задания
- Используйте in-memory хранилище (списки, словари) или простую БД (SQLite)
- Реализуйте обработку ошибок с правильными HTTP статус-кодами
- Используйте DTO (Data Transfer Objects) для передачи данных

### 3. Реализация аутентификации

- Реализуйте простую аутентификацию (можно использовать JWT токены или session-based)
- Защитите минимум 2 endpoint с помощью аутентификации
- Реализуйте endpoint для регистрации/логина пользователя
- Добавьте middleware для проверки аутентификации

### 4. Документирование API

- Создайте OpenAPI/Swagger спецификацию для вашего API
- Опишите все endpoints с параметрами, request/response схемами
- Добавьте примеры запросов и ответов
- Если возможно, добавьте Swagger UI для интерактивного тестирования API

### 5. Тестирование

- Создайте простые тесты для основных endpoints (можно использовать curl, Postman или unit-тесты)
- Протестируйте успешные сценарии
- Протестируйте обработку ошибок (невалидные данные, отсутствующие ресурсы и т.д.)

---

## Архитектура

```
nginx:8080  ──┬──▶  user-service:8081    /api/v1/auth, /api/v1/users, /api/v1/notifications
              ├──▶  catalog-service:8082  /api/v1/services
              └──▶  order-service:8083    /api/v1/orders
```

## Запуск

### Docker

```bash
docker compose up --build
# или
podman compose up --build
```

Swagger UI: http://localhost:8081/swagger

### Локально

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_UPDATES_DISCONNECTED=ON
bash apply_patches.sh
cmake --build build --target user_service catalog_service order_service
bash start.sh
```

**Важно:** `apply_patches.sh` нужно запускать после каждого вызова `cmake -B build ...`, так как cmake пересоздаёт `build/_deps/` и стирает патчи

- **Swagger UI: http://localhost:8080/swagger**
- user-service напрямую: http://localhost:8081
- catalog-service напрямую: http://localhost:8082
- order-service напрямую: http://localhost:8083

## Тесты

```bash
# Функциональные тесты
pip install pytest pytest-asyncio aiohttp
pytest tests/ -v

# Unit-тесты
cmake --build build --target profi_services_unittest
./build/profi_services_unittest
```
