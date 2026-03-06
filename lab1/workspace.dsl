workspace {
    !identifiers hierarchical
    !impliedRelationships false

    model {
        client = person "Клиент" "Пользователь, осуществляющий поиск и заказ услуг"
        provider = person "Исполнитель" "Пользователь, публикующий и предоставляющий услуги"

        emailService = softwareSystem "Email-сервис" "Отправка уведомлений по электронной почте" "External System"

        serviceOrderingSystem = softwareSystem "Платформа заказа услуг" "Поиск и заказ услуг клиентами, публикация услуг исполнителями" {
            webApp = container "Web-приложение" "Пользовательский интерфейс платформы" "React, Next.js" "Web Browser"
            userService = container "Сервис пользователей" "Регистрация, авторизация и поиск пользователей" "C++, userver"
            servicesCatalog = container "Сервис каталога услуг" "Создание, поиск и просмотр услуг" "C++, userver"
            orderService = container "Сервис заказов" "Создание заказов и управление статусами" "C++, userver"
            database = container "База данных" "Хранение пользователей, услуг и заказов" "PostgreSQL" "Database"

            webApp -> userService "Регистрация и поиск пользователей" "HTTPS/REST, JSON"
            webApp -> servicesCatalog "Просмотр и создание услуг" "HTTPS/REST, JSON"
            webApp -> orderService "Оформление и просмотр заказов" "HTTPS/REST, JSON"
            userService -> database "Чтение и запись данных пользователей" "SQL/TCP"
            servicesCatalog -> database "Чтение и запись данных услуг" "SQL/TCP"
            orderService -> database "Чтение и запись данных заказов" "SQL/TCP"
            orderService -> emailService "Уведомление исполнителя о заказе и клиента об оплате" "SMTP"
            servicesCatalog -> emailService "Уведомление клиентов о новых услугах" "SMTP"
        }

        client -> serviceOrderingSystem "Регистрация, поиск услуг, оформление заказа"
        provider -> serviceOrderingSystem "Регистрация, создание услуг"
        serviceOrderingSystem -> emailService "Отправка уведомлений" "SMTP"
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
            serviceOrderingSystem.webApp -> serviceOrderingSystem.servicesCatalog "Запрос списка услуг"
            serviceOrderingSystem.servicesCatalog -> serviceOrderingSystem.database "Чтение каталога услуг"
            client -> serviceOrderingSystem.webApp "Выбор услуги и оформление заказа"
            serviceOrderingSystem.webApp -> serviceOrderingSystem.orderService "Оформление заказа"
            serviceOrderingSystem.orderService -> serviceOrderingSystem.database "Сохранение заказа"
            serviceOrderingSystem.orderService -> emailService "Уведомление исполнителя о новом заказе"
            serviceOrderingSystem.orderService -> emailService "Уведомление клиента об успешном заказе"
            serviceOrderingSystem.orderService -> serviceOrderingSystem.webApp "Результат оформления заказа"
            serviceOrderingSystem.webApp -> client "Отображение результата оформления заказа"
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
        }
    }

}
