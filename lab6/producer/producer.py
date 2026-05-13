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
    credentials = pika.PlainCredentials(RABBITMQ_USER, RABBITMQ_PASS)
    params = pika.ConnectionParameters(
        host=RABBITMQ_HOST,
        credentials=credentials,
        heartbeat=600,
    )
    return pika.BlockingConnection(params)


def setup_topology(channel):
    channel.exchange_declare(
        exchange=EXCHANGE,
        exchange_type="topic",
        durable=True,
    )

    queues = {
        "notifications": [
            "user.registered",
            "order.created",
            "order.service_added",
            "order.status_changed",
        ],
        "analytics": ["#"],
        "cache_invalidation": [
            "service.created",
            "order.status_changed",
        ],
    }

    for queue_name, bindings in queues.items():
        channel.queue_declare(queue=queue_name, durable=True)
        for routing_key in bindings:
            channel.queue_bind(
                exchange=EXCHANGE,
                queue=queue_name,
                routing_key=routing_key,
            )

    print(f"Topology ready: exchange={EXCHANGE}, queues={list(queues.keys())}")


def publish_event(channel, routing_key, payload):
    event = {
        "event_id": str(uuid.uuid4()),
        "event_type": routing_key,
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "payload": payload,
    }

    channel.basic_publish(
        exchange=EXCHANGE,
        routing_key=routing_key,
        body=json.dumps(event, ensure_ascii=False),
        properties=pika.BasicProperties(
            delivery_mode=2,
            content_type="application/json",
        ),
    )

    print(f"[PUBLISHED] {routing_key}: {json.dumps(payload, ensure_ascii=False)}")


def main():
    connection = connect()
    channel = connection.channel()
    setup_topology(channel)

    print(f"\nPublishing {len(EVENTS)} events...\n")

    for event_def in EVENTS:
        publish_event(channel, event_def["routing_key"], event_def["payload"])
        time.sleep(1)

    print("\nAll events published. Waiting 30s before next cycle...\n")
    time.sleep(30)

    connection.close()


if __name__ == "__main__":
    while True:
        try:
            main()
        except pika.exceptions.AMQPConnectionError:
            print("RabbitMQ not ready, retrying in 5s...")
            time.sleep(5)
        except KeyboardInterrupt:
            break
