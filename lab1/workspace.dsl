workspace {
    !identifiers hierarchical
    !impliedRelationships false

    model {
        client = person "Клиент" "Пользователь, осуществляющий поиск и заказ услуг"
        provider = person "Исполнитель" "Пользователь, публикующий и предоставляющий услуги"

        emailService = softwareSystem "Email-сервис" "Отправка уведомлений по электронной почте" "External System"

        serviceOrderingSystem = softwareSystem "Платформа заказа услуг" "Поиск и заказ услуг клиентами, публикация услуг исполнителями" {
            webApp = container "Web-приложение" "Пользовательский интерфейс платформы" "React, Next.js" "Web Browser"
            apiGateway = container "API Gateway" "Маршрутизация запросов, аутентификация" "Kong / Nginx"
            userService = container "Сервис пользователей" "Регистрация, авторизация и поиск пользователей" "C++, userver"
            servicesCatalog = container "Сервис каталога услуг" "Создание, поиск и просмотр услуг" "C++, userver"
            orderService = container "Сервис заказов" "Создание заказов и управление статусами" "C++, userver"
            notificationService = container "Сервис уведомлений" "Обработка событий и отправка email-уведомлений" "C++, userver"
            messageBroker = container "Message Broker" "Асинхронная передача событий между сервисами" "Kafka" "Message Broker"
            userDb = container "БД пользователей" "Хранение данных пользователей" "PostgreSQL" "Database"
            catalogDb = container "БД каталога услуг" "Хранение данных услуг" "PostgreSQL" "Database"
            orderDb = container "БД заказов" "Хранение данных заказов" "PostgreSQL" "Database"

            webApp -> apiGateway "Все запросы" "HTTPS/REST, JSON"
            apiGateway -> userService "Регистрация и поиск пользователей" "HTTPS/REST, JSON"
            apiGateway -> servicesCatalog "Просмотр и создание услуг" "HTTPS/REST, JSON"
            apiGateway -> orderService "Оформление и просмотр заказов" "HTTPS/REST, JSON"
            userService -> userDb "Чтение и запись данных пользователей" "SQL/TCP"
            servicesCatalog -> catalogDb "Чтение и запись данных услуг" "SQL/TCP"
            orderService -> orderDb "Чтение и запись данных заказов" "SQL/TCP"
            orderService -> messageBroker "Публикация событий заказа" "Kafka protocol"
            servicesCatalog -> messageBroker "Публикация событий каталога" "Kafka protocol"
            messageBroker -> notificationService "Доставка событий для уведомлений" "Kafka protocol"
            notificationService -> emailService "Отправка email-уведомлений" "SMTP"
        }

        client -> serviceOrderingSystem "Регистрация, поиск услуг, оформление заказа"
        provider -> serviceOrderingSystem "Регистрация, создание услуг"
        client -> serviceOrderingSystem.webApp "Работа с платформой" "HTTPS"
        provider -> serviceOrderingSystem.webApp "Управление услугами и заказами" "HTTPS"
    }

    views {
        systemContext serviceOrderingSystem "C1" {
            include *
            autoLayout
        }

        container serviceOrderingSystem "C2" {
            include *
            autoLayout
        }

        dynamic serviceOrderingSystem "PlaceOrder" {
            description "Сценарий: оформление заказа клиентом"
            client -> serviceOrderingSystem.webApp "Просмотр каталога услуг"
            serviceOrderingSystem.webApp -> serviceOrderingSystem.apiGateway "Запрос списка услуг"
            serviceOrderingSystem.apiGateway -> serviceOrderingSystem.servicesCatalog "Запрос списка услуг"
            serviceOrderingSystem.servicesCatalog -> serviceOrderingSystem.catalogDb "Чтение каталога услуг"
            client -> serviceOrderingSystem.webApp "Выбор услуги и оформление заказа"
            serviceOrderingSystem.webApp -> serviceOrderingSystem.apiGateway "Оформление заказа"
            serviceOrderingSystem.apiGateway -> serviceOrderingSystem.orderService "Оформление заказа"
            serviceOrderingSystem.orderService -> serviceOrderingSystem.orderDb "Сохранение заказа"
            serviceOrderingSystem.orderService -> serviceOrderingSystem.messageBroker "Публикация события о новом заказе"
            serviceOrderingSystem.messageBroker -> serviceOrderingSystem.notificationService "Доставка события"
            serviceOrderingSystem.notificationService -> emailService "Уведомление исполнителя и клиента"
            autoLayout
        }

        theme default

        styles {
            element "Person" {
                shape Person
                background #08427B
                color #ffffff
            }
            element "Software System" {
                background #1168BD
                color #ffffff
            }
            element "External System" {
                background #999999
                color #ffffff
            }
            element "Container" {
                background #438DD5
                color #ffffff
            }
            element "Database" {
                shape Cylinder
                background #438DD5
                color #ffffff
            }
            element "Web Browser" {
                shape WebBrowser
                background #438DD5
                color #ffffff
            }
            element "Message Broker" {
                shape Pipe
                background #438DD5
                color #ffffff
            }
        }
    }

}
