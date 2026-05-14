use profidb

db.createCollection("services", {
    validator: {
        $jsonSchema: {
            bsonType: "object",
            required: ["title", "price", "provider_id", "created_at"],
            properties: {
                title: {
                    bsonType: "string",
                    minLength: 1,
                    maxLength: 256
                },
                description: {
                    bsonType: "string",
                    maxLength: 4096
                },
                price: {
                    bsonType: "double",
                    minimum: 0
                },
                provider_id: {
                    bsonType: "string",
                    minLength: 1
                },
                created_at: {
                    bsonType: "date"
                }
            }
        }
    },
    validationLevel: "strict",
    validationAction: "error"
});

print("Validation schema applied to services");

print("\n--- Тест 1: отрицательная цена ---");
try {
    db.services.insertOne({
        title: "Плохая услуга",
        price: -100,
        provider_id: "1",
        created_at: new Date()
    });
    print("FAIL: документ принят, а не должен был");
} catch (e) {
    print("OK: " + e.message);
}

print("\n--- Тест 2: пустой title ---");
try {
    db.services.insertOne({
        title: "",
        price: 1000,
        provider_id: "1",
        created_at: new Date()
    });
    print("FAIL: документ принят, а не должен был");
} catch (e) {
    print("OK: " + e.message);
}

print("\n--- Тест 3: нет поля price ---");
try {
    db.services.insertOne({
        title: "Без цены",
        provider_id: "1",
        created_at: new Date()
    });
    print("FAIL: документ принят, а не должен был");
} catch (e) {
    print("OK: " + e.message);
}

print("\n--- Тест 4: валидный документ ---");
try {
    const result = db.services.insertOne({
        title: "Тестовая услуга",
        description: "Описание тестовой услуги",
        price: 1000,
        provider_id: "1",
        created_at: new Date()
    });
    db.services.deleteOne({ _id: result.insertedId });
    print("OK: валидный документ принят и удалён");
} catch (e) {
    print("FAIL: " + e.message);
}
