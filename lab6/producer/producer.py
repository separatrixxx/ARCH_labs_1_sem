import json
import os
import time
import uuid
from datetime import datetime, timezone

import pika

RABBITMQ_HOST = os.getenv("RABBITMQ_HOST", "localhost")
RABBITMQ_USER = os.getenv("RABBITMQ_USER", "profi")
RABBITMQ_PASS = os.getenv("RABBITMQ_PASS", "profi")

EXCHANGE = "profi.events"

EVENTS = [
    {
        "routing_key": "user.registered",
        "payload": {
            "user_id": "13",
            "login": "test.user",
            "email": "test.user@example.com",
            "first_name": "Тест",
            "last_name": "Пользователь",
        },
    },
    {
        "routing_key": "user.registered",
        "payload": {
            "user_id": "14",
            "login": "another.user",
            "email": "another@example.com",
            "first_name": "Ещё",
            "last_name": "Один",
        },
    },
    {
        "routing_key": "service.created",
        "payload": {
            "service_id": "660000000000000000000101",
            "title": "Дизайн логотипа",
            "price": 5000.0,
            "provider_id": "13",
        },
    },
    {
        "routing_key": "order.created",
        "payload": {
            "order_id": "101",
            "client_id": "14",
            "status": "pending",
        },
    },
    {
        "routing_key": "order.service_added",
        "payload": {
            "order_id": "101",
            "service_id": "660000000000000000000101",
        },
    },
    {
        "routing_key": "order.status_changed",
        "payload": {
            "order_id": "101",
            "old_status": "pending",
            "new_status": "confirmed",
        },
    },
]


def connect():
    creds = pika.PlainCredentials(RABBITMQ_USER, RABBITMQ_PASS)
    params = pika.ConnectionParameters(
        host=RABBITMQ_HOST, credentials=creds, heartbeat=600,
    )
    return pika.BlockingConnection(params)


def setup_topology(ch):
    ch.exchange_declare(exchange=EXCHANGE, exchange_type="topic", durable=True)

    queues = {
        "notifications": [
            "user.registered", "order.created",
            "order.service_added", "order.status_changed",
        ],
        "analytics": ["#"],
        "cache_invalidation": ["service.created", "order.status_changed"],
    }

    for q_name, keys in queues.items():
        ch.queue_declare(queue=q_name, durable=True)
        for rk in keys:
            ch.queue_bind(exchange=EXCHANGE, queue=q_name, routing_key=rk)

    print(f"Topology ready: exchange={EXCHANGE}, queues={list(queues.keys())}")


def publish_event(ch, routing_key, payload):
    event = {
        "event_id": str(uuid.uuid4()),
        "event_type": routing_key,
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "payload": payload,
    }
    body = json.dumps(event, ensure_ascii=False)

    ch.basic_publish(
        exchange=EXCHANGE,
        routing_key=routing_key,
        body=body,
        properties=pika.BasicProperties(
            delivery_mode=2,
            content_type="application/json",
        ),
    )
    print(f"[PUBLISHED] {routing_key}: {json.dumps(payload, ensure_ascii=False)}")


def main():
    conn = connect()
    ch = conn.channel()
    setup_topology(ch)

    print(f"\nPublishing {len(EVENTS)} events...\n")

    for ev in EVENTS:
        publish_event(ch, ev["routing_key"], ev["payload"])
        time.sleep(1)

    print("\nDone. Waiting 30s before next cycle...\n")
    time.sleep(30)
    conn.close()


if __name__ == "__main__":
    while True:
        try:
            main()
        except pika.exceptions.AMQPConnectionError:
            print("RabbitMQ not ready, retrying in 5s...")
            time.sleep(5)
        except KeyboardInterrupt:
            break
