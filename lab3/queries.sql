-- POST /api/v1/auth/register
INSERT INTO users (login, email, first_name, last_name, password_hash)
VALUES ($1, $2, $3, $4, $5)
RETURNING id, login, email, first_name, last_name;

-- POST /api/v1/auth/login
SELECT id, login, email, first_name, last_name, password_hash
FROM users
WHERE login = $1;

-- GET /api/v1/users
SELECT id, login, email, first_name, last_name
FROM users
ORDER BY id;

-- GET /api/v1/users?login=...
SELECT id, login, email, first_name, last_name
FROM users
WHERE login = $1;

-- GET /api/v1/users?first_name=...&last_name=...
SELECT id, login, email, first_name, last_name
FROM users
WHERE ($1 = '' OR lower(first_name) LIKE lower($1))
  AND ($2 = '' OR lower(last_name)  LIKE lower($2));

-- POST /api/v1/services
INSERT INTO services (title, description, price, provider_id)
VALUES ($1, $2, $3, $4)
RETURNING id, title, description, price, provider_id;

-- GET /api/v1/services
SELECT id, title, description, price, provider_id
FROM services
ORDER BY id;

-- GET /api/v1/services/{id}
SELECT id, title, description, price, provider_id
FROM services
WHERE id = $1;

-- POST /api/v1/orders
INSERT INTO orders (client_id)
VALUES ($1)
RETURNING id, client_id, status;

-- GET /api/v1/orders
SELECT o.id, o.client_id, o.status,
       COALESCE(
           array_agg(os.service_id ORDER BY os.service_id) FILTER (WHERE os.service_id IS NOT NULL),
           '{}'::BIGINT[]
       ) AS service_ids
FROM orders o
LEFT JOIN order_services os ON os.order_id = o.id
WHERE o.client_id = $1
GROUP BY o.id
ORDER BY o.id;

-- GET /api/v1/orders/{id}
SELECT o.id, o.client_id, o.status,
       COALESCE(
           array_agg(os.service_id ORDER BY os.service_id) FILTER (WHERE os.service_id IS NOT NULL),
           '{}'::BIGINT[]
       ) AS service_ids
FROM orders o
LEFT JOIN order_services os ON os.order_id = o.id
WHERE o.id = $1
GROUP BY o.id;

-- POST /api/v1/orders/{order_id}/services
INSERT INTO order_services (order_id, service_id)
VALUES ($1, $2)
ON CONFLICT DO NOTHING;

SELECT o.id, o.client_id, o.status,
       COALESCE(
           array_agg(os.service_id ORDER BY os.service_id) FILTER (WHERE os.service_id IS NOT NULL),
           '{}'::BIGINT[]
       ) AS service_ids
FROM orders o
LEFT JOIN order_services os ON os.order_id = o.id
WHERE o.id = $1
GROUP BY o.id;

SELECT s.id, s.title, s.price, COUNT(os.order_id) AS order_count
FROM services s
JOIN order_services os ON os.service_id = s.id
GROUP BY s.id
ORDER BY order_count DESC
LIMIT 5;

SELECT u.login, u.first_name, u.last_name,
       COUNT(o.id)        AS total_orders,
       SUM(s.price)       AS total_spent
FROM users u
JOIN orders o          ON o.client_id = u.id
JOIN order_services os ON os.order_id = o.id
JOIN services s        ON s.id = os.service_id
WHERE o.status = 'completed'
GROUP BY u.id
ORDER BY total_spent DESC;

SELECT s.id, s.title, s.price,
       COUNT(os.order_id) AS times_ordered
FROM services s
LEFT JOIN order_services os ON os.service_id = s.id
WHERE s.provider_id = $1
GROUP BY s.id
ORDER BY times_ordered DESC;
