cmake_minimum_required(VERSION 3.12...3.31)
project(profi_services CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

find_package(userver COMPONENTS core QUIET)
if(NOT userver_FOUND)
  download_userver(TRY_DIR third_party/userver GIT_TAG 1e560caf6787b61b389032b7bec68f2d404f80b5)
endif()

userver_setup_environment()

# Shared JWT library
add_library(profi-shared STATIC src/shared/jwt.cpp)
target_link_libraries(profi-shared PUBLIC userver::core)
target_include_directories(profi-shared PUBLIC src/shared)

# ── user-service ─────────────────────────────────────────────────────────────
add_executable(user_service
    src/user-service/main.cpp
    src/user-service/storage.cpp
    src/user-service/handlers/auth_handler.cpp
    src/user-service/handlers/users_handler.cpp
    src/user-service/handlers/swagger_handler.cpp
    src/user-service/handlers/email_handler.cpp
)
target_link_libraries(user_service PRIVATE profi-shared)
target_include_directories(user_service PRIVATE
    src/user-service
    src/shared
)

# ── catalog-service ───────────────────────────────────────────────────────────
add_executable(catalog_service
    src/catalog-service/main.cpp
    src/catalog-service/storage.cpp
    src/catalog-service/handlers/services_handler.cpp
)
target_link_libraries(catalog_service PRIVATE profi-shared)
target_include_directories(catalog_service PRIVATE
    src/catalog-service
    src/shared
)

# ── order-service ─────────────────────────────────────────────────────────────
add_executable(order_service
    src/order-service/main.cpp
    src/order-service/storage.cpp
    src/order-service/handlers/orders_handler.cpp
)
target_link_libraries(order_service PRIVATE profi-shared)
target_include_directories(order_service PRIVATE
    src/order-service
    src/shared
)

# ── Unit tests (user-service storage) ────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/tests/storage_test.cpp")
    add_executable(profi_services_unittest
        src/tests/storage_test.cpp
        src/user-service/storage.cpp
    )
    target_link_libraries(profi_services_unittest PRIVATE profi-shared userver::utest)
    target_include_directories(profi_services_unittest PRIVATE src/user-service src/shared)
    add_google_tests(profi_services_unittest)
endif()
