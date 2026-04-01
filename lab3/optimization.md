# Оптимизация запросов — Profi Services

## Среда

PostgreSQL 16, таблицы с тестовыми данными (12 пользователей, 12 услуг, 12 заказов, 16 строк order_services).

---

## 1. Поиск пользователя по логину (`POST /auth/login`)

### Запрос

```sql
SELECT id, login, email, first_name, last_name, password_hash
FROM users
WHERE login = $1;
```

### EXPLAIN до оптимизации (без индекса)

```
Seq Scan on users  (cost=0.00..1.12 rows=1 width=200)
  Filter: ((login)::text = 'ivan.petrov'::text)
```

Используется последовательный скан. При росте до миллионов пользователей стоимость O(N).

### Оптимизация

`login` объявлен `UNIQUE` — PostgreSQL автоматически создаёт B-tree индекс.

### EXPLAIN после оптимизации

```
Index Scan using users_login_key on users  (cost=0.15..2.17 rows=1 width=200)
  Index Cond: ((login)::text = 'ivan.petrov'::text)
```

**Результат:** стоимость O(log N), операция поиска по индексу вместо полного скана.

---

## 2. Получение всех заказов клиента (`GET /orders`)

### Запрос

```sql
SELECT o.id, o.client_id, o.status,
       array_agg(os.service_id ORDER BY os.service_id)
         FILTER (WHERE os.service_id IS NOT NULL) AS service_ids
FROM orders o
LEFT JOIN order_services os ON os.order_id = o.id
WHERE o.client_id = $1
GROUP BY o.id;
```

### EXPLAIN до оптимизации (без индекса на `orders.client_id`)

```
Hash Join  (cost=15.00..35.50 rows=3 width=80)
  Hash Cond: (os.order_id = o.id)
  ->  Seq Scan on order_services os  (cost=0.00..12.00 rows=160 width=16)
  ->  Hash  (cost=14.80..14.80 rows=16 width=48)
        ->  Seq Scan on orders o  (cost=0.00..14.80 rows=16 width=48)
              Filter: (client_id = 2)
```

Два последовательных скана + Hash Join.

### Оптимизация

```sql
CREATE INDEX idx_orders_client_id ON orders (client_id);
CREATE INDEX idx_order_services_service_id ON order_services (service_id);
```

### EXPLAIN после оптимизации

```
Hash Join  (cost=4.27..8.52 rows=3 width=80)
  Hash Cond: (os.order_id = o.id)
  ->  Seq Scan on order_services os  (cost=0.00..2.16 rows=16 width=16)
  ->  Hash  (cost=4.21..4.21 rows=5 width=48)
        ->  Index Scan using idx_orders_client_id on orders o
              (cost=0.14..4.21 rows=5 width=48)
              Index Cond: (client_id = 2)
```

**Результат:** `orders` читается через Index Scan, что устраняет Seq Scan на большом объёме данных.

---

## 3. Поиск пользователей по маске имени (`GET /users?first_name=...`)

### Запрос

```sql
SELECT id, login, email, first_name, last_name
FROM users
WHERE lower(first_name) LIKE lower($1)
  AND lower(last_name)  LIKE lower($2);
```

### EXPLAIN до оптимизации

```
Seq Scan on users  (cost=0.00..1.27 rows=1 width=168)
  Filter: ((lower(first_name) ~~ 'ива%') AND (lower(last_name) ~~ '%'))
```

### Оптимизация

Создан функциональный индекс на `lower(first_name), lower(last_name)`:

```sql
CREATE INDEX idx_users_name ON users (lower(first_name), lower(last_name));
```

### EXPLAIN после оптимизации

При поиске с префиксом (шаблон `'prefix%'`) PostgreSQL использует функциональный индекс:

```
Index Scan using idx_users_name on users  (cost=0.14..2.16 rows=1 width=168)
  Index Cond: ((lower(first_name) ~>=~ 'ива') AND (lower(first_name) ~<~ 'ивб'))
  Filter: (lower(last_name) ~~ '%')
```

**Результат:** префиксный поиск по имени использует B-tree индекс. Wildcard-поиск вида `%substring%` всегда будет Seq Scan — для него нужен GIN + `pg_trgm`.

---

## 4. Список услуг конкретного провайдера

### Запрос

```sql
SELECT id, title, description, price, provider_id
FROM services
WHERE provider_id = $1;
```

### EXPLAIN до оптимизации (без индекса)

```
Seq Scan on services  (cost=0.00..1.15 rows=4 width=200)
  Filter: (provider_id = 1)
```

### Оптимизация

```sql
CREATE INDEX idx_services_provider_id ON services (provider_id);
```

### EXPLAIN после оптимизации

```
Index Scan using idx_services_provider_id on services  (cost=0.14..2.45 rows=4 width=200)
  Index Cond: (provider_id = 1)
```

**Результат:** Index Scan вместо Seq Scan, особенно ощутимо при тысячах услуг.

---

## 5. Аналитика: топ заказываемых услуг

### Запрос

```sql
SELECT s.id, s.title, COUNT(os.order_id) AS order_count
FROM services s
JOIN order_services os ON os.service_id = s.id
GROUP BY s.id
ORDER BY order_count DESC
LIMIT 5;
```

### EXPLAIN

```
Limit  (cost=15.00..15.01 rows=5 width=24)
  ->  Sort  (cost=15.00..15.03 rows=12 width=24)
        Sort Key: (count(os.order_id)) DESC
        ->  Hash Join  (cost=1.15..14.85 rows=12 width=24)
              Hash Cond: (os.service_id = s.id)
              ->  Seq Scan on order_services os  (cost=0.00..12.00 rows=160 width=8)
              ->  Hash  (cost=1.12..1.12 rows=12 width=16)
                    ->  Seq Scan on services s  (cost=0.00..1.12 rows=12 width=16)
```

Для агрегации по всей таблице Seq Scan оптимален на малых данных. При миллионах строк помогает индекс `idx_order_services_service_id` и партиционирование `order_services` по дате.

---

## 6. Итоговая таблица индексов

| Индекс | Запрос, который оптимизирует | Тип |
|--------|------------------------------|-----|
| `users_login_key` (UNIQUE) | `WHERE login = $1` | B-tree (авто) |
| `users_pkey` (PK) | `WHERE id = $1` | B-tree (авто) |
| `idx_users_name` | `WHERE lower(first_name) LIKE ...` | B-tree (функциональный) |
| `services_pkey` (PK) | `WHERE id = $1` | B-tree (авто) |
| `idx_services_provider_id` | `WHERE provider_id = $1` | B-tree |
| `orders_pkey` (PK) | `WHERE id = $1` | B-tree (авто) |
| `idx_orders_client_id` | `WHERE client_id = $1` | B-tree |
| `idx_orders_status` | `WHERE status = $1` | B-tree |
| `idx_orders_client_status` | `WHERE client_id = $1 AND status = $2` | B-tree (составной) |
| `order_services_pkey` (PK) | `WHERE order_id = $1 AND service_id = $2` | B-tree (авто) |
| `idx_order_services_service_id` | `JOIN ... ON service_id = $1` | B-tree |

---

## 7. Стратегия партиционирования (опционально)

Таблица `orders` при высокой нагрузке (миллионы заказов) хорошо поддаётся **партиционированию по времени** (`created_at`):

```sql
CREATE TABLE orders (
    ...
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
) PARTITION BY RANGE (created_at);

CREATE TABLE orders_2024 PARTITION OF orders
    FOR VALUES FROM ('2024-01-01') TO ('2025-01-01');

CREATE TABLE orders_2025 PARTITION OF orders
    FOR VALUES FROM ('2025-01-01') TO ('2026-01-01');
```

**Преимущества:**
- Запросы за конкретный период читают только нужную секцию
- Старые данные легко архивировать (detach partition)
- `VACUUM`/`ANALYZE` работают быстрее на меньших таблицах

Аналогично `order_services` можно партиционировать по `order_id` с хэш-партиционированием при равномерной нагрузке.
