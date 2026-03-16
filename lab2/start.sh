#!/bin/bash
pkill -f "./build/user_service" 2>/dev/null
pkill -f "./build/catalog_service" 2>/dev/null
pkill -f "./build/order_service" 2>/dev/null
nginx -c "$(pwd)/configs/nginx/nginx-local.conf" -p "$(pwd)" -s stop 2>/dev/null || true
sleep 1

./build/user_service --config configs/user-service/static_config.yaml > /tmp/user_service.log 2>&1 &
./build/catalog_service --config configs/catalog-service/static_config.yaml > /tmp/catalog_service.log 2>&1 &
./build/order_service --config configs/order-service/static_config.yaml > /tmp/order_service.log 2>&1 &
nginx -c "$(pwd)/configs/nginx/nginx-local.conf" -p "$(pwd)"

echo "Services started. Swagger UI: http://localhost:8080/swagger"
