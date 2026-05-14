use profidb

db.services.insertOne({
    title: "Курьерская доставка",
    description: "Доставка в пределах города, до 10 кг",
    price: 500,
    provider_id: "1",
    created_at: new Date()
});

db.services.find({}).sort({ _id: 1 });

db.services.findOne({ _id: ObjectId("660000000000000000000001") });

db.services.find({ price: { $gt: 2000, $lt: 10000 } }).sort({ price: 1 });

db.services.find({ provider_id: "1" });

db.services.find({ title: { $regex: "ремонт", $options: "i" } });

db.services.find({ price: { $in: [2500, 3000, 5000] } });

db.services.find({
    $or: [
        { price: { $lt: 2000 } },
        { price: { $gt: 10000 } }
    ]
});

db.services.updateOne(
    { _id: ObjectId("660000000000000000000001") },
    { $set: { price: 2800 } }
);

db.services.updateMany(
    { price: { $lt: 2000 } },
    [{ $set: { price: { $multiply: ["$price", 1.1] } } }]
);

db.services.updateOne(
    { title: "Настройка сети" },
    { $set: { description: "Wi-Fi, VPN, проброс портов, настройка firewall" } }
);

db.services.deleteOne({ title: "Курьерская доставка" });

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

db.services.aggregate([
    { $sort: { price: -1 } },
    { $limit: 3 },
    { $project: { title: 1, price: 1, provider_id: 1 } }
]);
