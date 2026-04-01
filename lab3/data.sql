INSERT INTO users (login, email, first_name, last_name, password_hash) VALUES
    ('ivan.petrov',    'ivan.petrov@example.com',    'Иван',      'Петров',     'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
    ('maria.ivanova',  'maria.ivanova@example.com',  'Мария',     'Иванова',    'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
    ('alex.sidorov',   'alex.sidorov@example.com',   'Алексей',   'Сидоров',    'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
    ('olga.kozlova',   'olga.kozlova@example.com',   'Ольга',     'Козлова',    'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
    ('dmitry.novikov', 'dmitry.novikov@example.com', 'Дмитрий',   'Новиков',    'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
    ('elena.morozova', 'elena.morozova@example.com', 'Елена',     'Морозова',   'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
    ('sergey.volkov',  'sergey.volkov@example.com',  'Сергей',    'Волков',     'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
    ('anna.sokolova',  'anna.sokolova@example.com',  'Анна',      'Соколова',   'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
    ('mikhail.lebedev','mikhail.lebedev@example.com','Михаил',    'Лебедев',    'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
    ('natasha.popova', 'natasha.popova@example.com', 'Наталья',   'Попова',     'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
    ('andrey.zaitsev', 'andrey.zaitsev@example.com', 'Андрей',    'Зайцев',     'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
    ('yulia.sorokina', 'yulia.sorokina@example.com', 'Юлия',      'Сорокина',   'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f');

INSERT INTO services (title, description, price, provider_id) VALUES
    ('Ремонт компьютеров',          'Диагностика, замена комплектующих, установка ПО',            2500.00, 1),
    ('Настройка сети',              'Прокладка кабеля, настройка Wi-Fi, VPN',                     3000.00, 1),
    ('Разработка сайта (landing)',  'Одностраничный сайт под ключ, адаптивный дизайн',           15000.00, 3),
    ('SEO-продвижение',             'Аудит сайта, подбор ключевых слов, оптимизация',             8000.00, 3),
    ('Уборка квартиры (стандарт)', 'Влажная уборка, пылесосить, мытьё полов',                    2000.00, 5),
    ('Генеральная уборка',         'Полная уборка включая окна, холодильник, плиту',              5000.00, 5),
    ('Репетитор по математике',    'Подготовка к ЕГЭ/ОГЭ, решение задач',                        1500.00, 7),
    ('Репетитор по английскому',   'Разговорный English, грамматика, IELTS',                      1800.00, 7),
    ('Фотосессия (портрет)',        'Выездная фотосессия, 50 обработанных фото',                  7000.00, 11),
    ('Видеосъёмка мероприятий',    'Полный день съёмки, монтаж ролика до 5 минут',               20000.00, 11),
    ('Юридическая консультация',   'Консультация по гражданскому праву, 1 час',                   3500.00, 3),
    ('Бухгалтерские услуги',       'Ведение ИП, сдача отчётности в ИФНС',                        5000.00, 1);

INSERT INTO orders (client_id, status) VALUES
    (2,  'completed'),
    (2,  'confirmed'),
    (4,  'pending'),
    (4,  'completed'),
    (6,  'pending'),
    (6,  'cancelled'),
    (8,  'confirmed'),
    (8,  'completed'),
    (10, 'pending'),
    (10, 'confirmed'),
    (12, 'pending'),
    (12, 'completed');

INSERT INTO order_services (order_id, service_id) VALUES
    (1,  1),
    (1,  2),
    (2,  3),
    (3,  5),
    (4,  6),
    (4,  5),
    (5,  7),
    (6,  8),
    (7,  9),
    (7, 10),
    (8,  4),
    (9, 11),
    (10, 12),
    (11,  1),
    (12,  3),
    (12,  4);
