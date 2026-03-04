workspace {

    model {
        client = person "Клиент" "Пользователь, осуществляющий поиск и заказ услуг"
        provider = person "Исполнитель" "Пользователь, публикующий и предоставляющий услуги"

        paymentSystem = softwareSystem "Платёжная система" "Обработка платежей за услуги" "External System"
        emailService = softwareSystem "Email-сервис" "Отправка уведомлений по электронной почте" "External System"

        serviceOrderingSystem = softwareSystem "Платформа заказа услуг" "Поиск и заказ услуг клиентами, публикация услуг исполнителями" {
            webApp = container "Web-приложение" "Пользовательский интерфейс платформы" "React, Next.js" "Web Browser"
            userService = container "Сервис пользователей" "Регистрация, авторизация и поиск пользователей" "C++, userver"
            servicesCatalog = container "Сервис каталога услуг" "Создание, поиск и просмотр услуг" "C++, userver"
            orderService = container "Сервис заказов" "Создание заказов и управление статусами" "C++, userver"
            database = container "База данных" "Хранение пользователей, услуг и заказов" "PostgreSQL" "Database"
        }

        client -> serviceOrderingSystem "Регистрация, поиск услуг, оформление и оплата заказа"
        provider -> serviceOrderingSystem "Регистрация, создание услуг"
        serviceOrderingSystem -> paymentSystem "Обработка платежей" "HTTPS/REST"
        serviceOrderingSystem -> emailService "Отправка уведомлений" "SMTP"

        client -> webApp "Работа с платформой" "HTTPS"
        provider -> webApp "Управление услугами и заказами" "HTTPS"

        webApp -> userService "Регистрация и поиск пользователей" "HTTPS/REST, JSON"
        webApp -> servicesCatalog "Просмотр и создание услуг" "HTTPS/REST, JSON"
        webApp -> orderService "Оформление и просмотр заказов" "HTTPS/REST, JSON"

        userService -> database "Чтение и запись данных пользователей" "SQL/TCP"
        servicesCatalog -> database "Чтение и запись данных услуг" "SQL/TCP"
        orderService -> database "Чтение и запись данных заказов" "SQL/TCP"
        orderService -> paymentSystem "Инициирование оплаты" "HTTPS/REST"
        orderService -> emailService "Уведомление исполнителя о заказе и клиента об оплате" "SMTP"
        servicesCatalog -> emailService "Уведомление клиентов о новых услугах" "SMTP"
    }

    views {
        systemContext serviceOrderingSystem "SystemContext" {
            include *
            autoLayout
        }

        container serviceOrderingSystem "Containers" {
            include *
            autoLayout
        }

        dynamic serviceOrderingSystem "PlaceOrder" "Оформление и оплата заказа клиентом" {
            client -> webApp "Выбор услуги и оформление заказа"
            webApp -> orderService "POST /orders?serviceId={id}"
            orderService -> database "Сохранение заказа"
            orderService -> paymentSystem "Оплата заказа"
            paymentSystem -> orderService "Результат оплаты"
            orderService -> emailService "Уведомление исполнителя о новом заказе"
            orderService -> emailService "Уведомление клиента об успешной оплате"
            orderService -> webApp "Результат оформления заказа"
            webApp -> client "Отображение результата оформления заказа"
            autoLayout
        }

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
