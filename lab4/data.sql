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

-- service_id ссылается на MongoDB ObjectId из коллекции services
-- Те же фиксированные ObjectId, что в data.js
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
    (1,  '660000000000000000000001'),
    (1,  '660000000000000000000002'),
    (2,  '660000000000000000000003'),
    (3,  '660000000000000000000005'),
    (4,  '660000000000000000000006'),
    (4,  '660000000000000000000005'),
    (5,  '660000000000000000000007'),
    (6,  '660000000000000000000008'),
    (7,  '660000000000000000000009'),
    (7,  '66000000000000000000000a'),
    (8,  '660000000000000000000004'),
    (9,  '66000000000000000000000b'),
    (10, '66000000000000000000000c'),
    (11, '660000000000000000000001'),
    (12, '660000000000000000000003'),
    (12, '660000000000000000000004');
