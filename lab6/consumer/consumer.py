import json
import os
import time

import pika

RABBITMQ_HOST = os.getenv("RABBITMQ_HOST", "localhost")
RABBITMQ_USER = os.getenv("RABBITMQ_USER", "profi")
RABBITMQ_PASS = os.getenv("RABBITMQ_PASS", "profi")
QUEUE = os.getenv("CONSUMER_QUEUE", "notifications")

HANDLERS = {
    "notifications": {
        "user.registered": lambda p: print(
            f"  -> Отправить welcome-email на {p['email']} для {p['first_name']} {p['last_name']}"
        ),
        "order.created": lambda p: print(
            f"  -> Уведомить клиента {p['client_id']}: заказ #{p['order_id']} создан"
        ),
        "order.service_added": lambda p: print(
            f"  -> Уведомить: услуга {p['service_id']} добавлена в заказ #{p['order_id']}"
        ),
        "order.status_changed": lambda p: print(
            f"  -> Уведомить: заказ #{p['order_id']} — {p['old_status']} -> {p['new_status']}"
        ),
    },
    "analytics": {
        "*": lambda event_type, p: print(
            f"  -> Записать в аналитику: {event_type} | {json.dumps(p, ensure_ascii=False)}"
        ),
    },
    "cache_invalidation": {
        "service.created": lambda p: print(
            f"  -> Инвалидировать кеш: DEL services:all (новая услуга: {p['title']})"
        ),
        "order.status_changed": lambda p: print(
            f"  -> Инвалидировать кеш: DEL orders:client:{p.get('client_id', '?')}"
        ),
    },
}


def on_message(channel, method, properties, body):
    event = json.loads(body)
    event_type = event.get("event_type", "unknown")
    payload = event.get("payload", {})
    timestamp = event.get("timestamp", "?")

    print(f"\n[{QUEUE.upper()}] Received: {event_type} at {timestamp}")

    queue_handlers = HANDLERS.get(QUEUE, {})

    if "*" in queue_handlers:
        queue_handlers["*"](event_type, payload)
    elif event_type in queue_handlers:
        queue_handlers[event_type](payload)
    else:
        print(f"  -> No handler for {event_type}, skipping")

    channel.basic_ack(delivery_tag=method.delivery_tag)


def main():
    credentials = pika.PlainCredentials(RABBITMQ_USER, RABBITMQ_PASS)
    params = pika.ConnectionParameters(
        host=RABBITMQ_HOST,
        credentials=credentials,
        heartbeat=600,
    )
    connection = pika.BlockingConnection(params)
    channel = connection.channel()

    channel.queue_declare(queue=QUEUE, durable=True)
    channel.basic_qos(prefetch_count=1)
    channel.basic_consume(queue=QUEUE, on_message_callback=on_message)

    print(f"[{QUEUE.upper()}] Consumer started. Waiting for events...")
    channel.start_consuming()


if __name__ == "__main__":
    while True:
        try:
            main()
        except pika.exceptions.AMQPConnectionError:
            print(f"[{QUEUE}] RabbitMQ not ready, retrying in 5s...")
            time.sleep(5)
        except KeyboardInterrupt:
            break
