CREATE TABLE IF NOT EXISTS users (
    id            BIGSERIAL     PRIMARY KEY,
    login         VARCHAR(64)   NOT NULL UNIQUE,
    email         VARCHAR(256)  NOT NULL UNIQUE,
    first_name    VARCHAR(128)  NOT NULL,
    last_name     VARCHAR(128)  NOT NULL,
    password_hash VARCHAR(256)  NOT NULL,
    created_at    TIMESTAMPTZ   NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS orders (
    id         BIGSERIAL   PRIMARY KEY,
    client_id  BIGINT      NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    status     VARCHAR(32) NOT NULL DEFAULT 'pending'
                           CHECK (status IN ('pending', 'confirmed', 'completed', 'cancelled')),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS order_services (
    order_id   BIGINT NOT NULL REFERENCES orders(id) ON DELETE CASCADE,
    service_id TEXT   NOT NULL,
    PRIMARY KEY (order_id, service_id)
);

CREATE INDEX IF NOT EXISTS idx_users_name
    ON users (lower(first_name), lower(last_name));

CREATE INDEX IF NOT EXISTS idx_orders_client_id
    ON orders (client_id);

CREATE INDEX IF NOT EXISTS idx_orders_status
    ON orders (status);

CREATE INDEX IF NOT EXISTS idx_orders_client_status
    ON orders (client_id, status);
