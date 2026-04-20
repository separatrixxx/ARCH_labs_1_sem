use profidb

// Каталог услуг хранится в MongoDB.
// provider_id ссылается на users.id в PostgreSQL (BIGSERIAL → строка).
// Фиксированные ObjectId для согласованности с data.sql (order_services).

const svcIds = [
    ObjectId("660000000000000000000001"),
    ObjectId("660000000000000000000002"),
    ObjectId("660000000000000000000003"),
    ObjectId("660000000000000000000004"),
    ObjectId("660000000000000000000005"),
    ObjectId("660000000000000000000006"),
    ObjectId("660000000000000000000007"),
    ObjectId("660000000000000000000008"),
    ObjectId("660000000000000000000009"),
    ObjectId("66000000000000000000000a"),
    ObjectId("66000000000000000000000b"),
    ObjectId("66000000000000000000000c"),
];

db.services.insertMany([
    { _id: svcIds[0],  title: "Ремонт компьютеров",         description: "Диагностика, замена комплектующих, установка ПО",           price: 2500,  provider_id: "1",  created_at: new Date("2024-01-20") },
    { _id: svcIds[1],  title: "Настройка сети",             description: "Прокладка кабеля, настройка Wi-Fi, VPN",                    price: 3000,  provider_id: "1",  created_at: new Date("2024-01-21") },
    { _id: svcIds[2],  title: "Разработка сайта (landing)", description: "Одностраничный сайт под ключ, адаптивный дизайн",          price: 15000, provider_id: "3",  created_at: new Date("2024-01-25") },
    { _id: svcIds[3],  title: "SEO-продвижение",            description: "Аудит сайта, подбор ключевых слов, оптимизация",            price: 8000,  provider_id: "3",  created_at: new Date("2024-01-26") },
    { _id: svcIds[4],  title: "Уборка квартиры (стандарт)",description: "Влажная уборка, пылесосить, мытьё полов",                   price: 2000,  provider_id: "5",  created_at: new Date("2024-02-02") },
    { _id: svcIds[5],  title: "Генеральная уборка",         description: "Полная уборка включая окна, холодильник, плиту",            price: 5000,  provider_id: "5",  created_at: new Date("2024-02-03") },
    { _id: svcIds[6],  title: "Репетитор по математике",   description: "Подготовка к ЕГЭ/ОГЭ, решение задач",                       price: 1500,  provider_id: "7",  created_at: new Date("2024-02-12") },
    { _id: svcIds[7],  title: "Репетитор по английскому",  description: "Разговорный English, грамматика, IELTS",                     price: 1800,  provider_id: "7",  created_at: new Date("2024-02-13") },
    { _id: svcIds[8],  title: "Фотосессия (портрет)",      description: "Выездная фотосессия, 50 обработанных фото",                 price: 7000,  provider_id: "11", created_at: new Date("2024-03-11") },
    { _id: svcIds[9],  title: "Видеосъёмка мероприятий",  description: "Полный день съёмки, монтаж ролика до 5 минут",              price: 20000, provider_id: "11", created_at: new Date("2024-03-12") },
    { _id: svcIds[10], title: "Юридическая консультация",  description: "Консультация по гражданскому праву, 1 час",                  price: 3500,  provider_id: "3",  created_at: new Date("2024-03-15") },
    { _id: svcIds[11], title: "Бухгалтерские услуги",      description: "Ведение ИП, сдача отчётности в ИФНС",                       price: 5000,  provider_id: "1",  created_at: new Date("2024-03-16") },
]);

db.services.createIndex({ provider_id: 1 });
db.services.createIndex({ price: 1 });

print("Services loaded: " + db.services.countDocuments() + " documents");
