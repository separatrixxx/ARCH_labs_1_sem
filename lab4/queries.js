use profidb

// Коллекция services — каталог услуг в MongoDB.
// Пользователи и заказы хранятся в PostgreSQL.

// ── CREATE ────────────────────────────────────────────────────────────────────

// POST /api/v1/services — создание услуги
db.services.insertOne({
    title: "Курьерская доставка",
    description: "Доставка в пределах города, до 10 кг",
    price: 500,
    provider_id: "1",   // ссылка на users.id в PostgreSQL
    created_at: new Date()
});

// ── READ ──────────────────────────────────────────────────────────────────────

// GET /api/v1/services — все услуги
db.services.find({}).sort({ _id: 1 });

// GET /api/v1/services/{id} — услуга по ID
db.services.findOne({ _id: ObjectId("660000000000000000000001") });

// Фильтрация по цене: $gt, $lt
db.services.find({ price: { $gt: 2000, $lt: 10000 } }).sort({ price: 1 });

// Услуги конкретного провайдера (provider_id = PG users.id)
db.services.find({ provider_id: "1" });

// Поиск услуг по названию ($regex, case-insensitive)
db.services.find({ title: { $regex: "ремонт", $options: "i" } });

// Услуги с ценой $in заданных диапазонов
db.services.find({ price: { $in: [2500, 3000, 5000] } });

// Услуги дешевле 2000 или дороже 10000 ($or)
db.services.find({
    $or: [
        { price: { $lt: 2000 } },
        { price: { $gt: 10000 } }
    ]
});

// ── UPDATE ────────────────────────────────────────────────────────────────────

// Изменить цену услуги
db.services.updateOne(
    { _id: ObjectId("660000000000000000000001") },
    { $set: { price: 2800 } }
);

// Массово поднять цены дешёвых услуг на 10%: price < 2000
db.services.updateMany(
    { price: { $lt: 2000 } },
    [{ $set: { price: { $multiply: ["$price", 1.1] } } }]
);

// Изменить описание
db.services.updateOne(
    { title: "Настройка сети" },
    { $set: { description: "Wi-Fi, VPN, проброс портов, настройка firewall" } }
);

// ── DELETE ────────────────────────────────────────────────────────────────────

// Удалить тестовую услугу
db.services.deleteOne({ title: "Курьерская доставка" });

// ── AGGREGATION ───────────────────────────────────────────────────────────────

// Количество и средняя цена услуг по провайдерам
db.services.aggregate([
    { $group: {
        _id: "$provider_id",
        service_count: { $sum: 1 },
        avg_price: { $avg: "$price" },
        min_price: { $min: "$price" },
        max_price: { $max: "$price" }
    }},
    { $sort: { service_count: -1 } }
]);

// Распределение услуг по ценовым категориям
db.services.aggregate([
    { $bucket: {
        groupBy: "$price",
        boundaries: [0, 2000, 5000, 10000, 50000],
        default: "50000+",
        output: {
            count: { $sum: 1 },
            titles: { $push: "$title" }
        }
    }}
]);

// Топ-3 самых дорогих услуги
db.services.aggregate([
    { $sort: { price: -1 } },
    { $limit: 3 },
    { $project: { title: 1, price: 1, provider_id: 1 } }
]);
