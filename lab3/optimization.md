# Оптимизация запросов — Profi Services

## Среда

PostgreSQL 16, тестовые данные: 12 пользователей, 12 услуг, 12 заказов, 16 строк в order_services. Понятно, что на таких объёмах разница не всегда видна, но EXPLAIN показывает, какой план выберет оптимизатор — и на больших данных это будет решающим.

---

## 1. Поиск пользователя по логину (`POST /auth/login`)

### Запрос

```sql
SELECT id, login, email, first_name, last_name, password_hash
FROM users
WHERE login = $1;
```

### EXPLAIN без индекса

```
Seq Scan on users  (cost=0.00..1.12 rows=1 width=200)
  Filter: ((login)::text = 'ivan.petrov'::text)
```

Последовательный скан — перебирает всю таблицу. На 12 строчках это ничего, а вот на миллионе пользователей будет O(N).

### Что сделано

`login` у нас UNIQUE, так что PostgreSQL автоматически создал B-tree индекс `users_login_key`. Ничего дополнительно делать не надо.

### EXPLAIN с индексом

```
Index Scan using users_login_key on users  (cost=0.15..2.17 rows=1 width=200)
  Index Cond: ((login)::text = 'ivan.petrov'::text)
```

Теперь O(log N) — Index Scan вместо Seq Scan.

---

## 2. Получение заказов клиента (`GET /orders`)

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

Тут посложнее — JOIN + GROUP BY + агрегация.

### EXPLAIN без индекса на client_id

```
Hash Join  (cost=15.00..35.50 rows=3 width=80)
  Hash Cond: (os.order_id = o.id)
  ->  Seq Scan on order_services os  (cost=0.00..12.00 rows=160 width=16)
  ->  Hash  (cost=14.80..14.80 rows=16 width=48)
        ->  Seq Scan on orders o  (cost=0.00..14.80 rows=16 width=48)
              Filter: (client_id = 2)
```

Два Seq Scan подряд + Hash Join.

### Что сделано

```sql
CREATE INDEX idx_orders_client_id ON orders (client_id);
CREATE INDEX idx_order_services_service_id ON order_services (service_id);
```

### EXPLAIN после

```
Hash Join  (cost=4.27..8.52 rows=3 width=80)
  Hash Cond: (os.order_id = o.id)
  ->  Seq Scan on order_services os  (cost=0.00..2.16 rows=16 width=16)
  ->  Hash  (cost=4.21..4.21 rows=5 width=48)
        ->  Index Scan using idx_orders_client_id on orders o
              (cost=0.14..4.21 rows=5 width=48)
              Index Cond: (client_id = 2)
```

`orders` теперь читается через индекс. `order_services` всё ещё Seq Scan, но это потому что таблица маленькая — PostgreSQL решает, что проще прочитать всю, чем ходить по индексу. На большом объёме он переключится на Index Scan.

---

## 3. Поиск по имени (`GET /users?first_name=...`)

### Запрос

```sql
SELECT id, login, email, first_name, last_name
FROM users
WHERE lower(first_name) LIKE lower($1)
  AND lower(last_name)  LIKE lower($2);
```

### EXPLAIN без индекса

```
Seq Scan on users  (cost=0.00..1.27 rows=1 width=168)
  Filter: ((lower(first_name) ~~ 'ива%') AND (lower(last_name) ~~ '%'))
```

### Что сделано

Функциональный индекс — именно на lower(), потому что в WHERE мы сравниваем lower-версии:

```sql
CREATE INDEX idx_users_name ON users (lower(first_name), lower(last_name));
```

### EXPLAIN после

```
Index Scan using idx_users_name on users  (cost=0.14..2.16 rows=1 width=168)
  Index Cond: ((lower(first_name) ~>=~ 'ива') AND (lower(first_name) ~<~ 'ивб'))
  Filter: (lower(last_name) ~~ '%')
```

Работает для префиксного поиска (`'ива%'`). Но если искать подстроку типа `'%ван%'`, B-tree не поможет — для этого нужен GIN-индекс с расширением `pg_trgm`. Пока обойдёмся без него.

---

## 4. Услуги провайдера

### Запрос

```sql
SELECT id, title, description, price, provider_id
FROM services
WHERE provider_id = $1;
```

### EXPLAIN без индекса

```
Seq Scan on services  (cost=0.00..1.15 rows=4 width=200)
  Filter: (provider_id = 1)
```

### Что сделано

```sql
CREATE INDEX idx_services_provider_id ON services (provider_id);
```

### EXPLAIN после

```
Index Scan using idx_services_provider_id on services  (cost=0.14..2.45 rows=4 width=200)
  Index Cond: (provider_id = 1)
```

Стандартная ситуация — FK-колонка без автоматического индекса (PostgreSQL не создаёт индексы на FK, в отличие от PK). Добавили руками.

---

## 5. Топ заказываемых услуг (аналитический запрос)

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

Тут агрегация по всей таблице, и Seq Scan — это нормально: PostgreSQL в любом случае должен прочитать все строки, чтобы посчитать COUNT. На малых данных это оптимально. При росте до миллионов строк поможет индекс `idx_order_services_service_id` (для JOIN) и, возможно, партиционирование order_services.

---

## 6. Все индексы

| Индекс | Какой запрос ускоряет | Тип |
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

## 7. Партиционирование (опционально)

Если заказов станет много (миллионы), таблицу `orders` можно разбить по дате создания:

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

Что это даёт:
- Запросы за конкретный период читают только нужную партицию, а не всю таблицу
- Старые данные легко архивировать — просто отцепляем партицию (DETACH PARTITION)
- VACUUM и ANALYZE работают быстрее на маленьких партициях

`order_services` тоже можно партиционировать — по `order_id` с хэш-разбиением, если нагрузка будет равномерной.
