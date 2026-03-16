#pragma once

#include <string>
#include <vector>

namespace profi::dto {

struct RegisterRequest {
    std::string login;
    std::string email;
    std::string first_name;
    std::string last_name;
    std::string password;
};

struct UserResponse {
    std::string id;
    std::string login;
    std::string email;
    std::string first_name;
    std::string last_name;
};

struct LoginRequest {
    std::string login;
    std::string password;
};

struct TokenResponse {
    std::string token;
};

struct CreateServiceRequest {
    std::string title;
    std::string description;
    double price{0.0};
};

struct ServiceResponse {
    std::string id;
    std::string title;
    std::string description;
    double price{0.0};
    std::string provider_id;
};

struct OrderResponse {
    std::string id;
    std::string client_id;
    std::string status;
    std::vector<std::string> service_ids;
};

struct AddServiceRequest {
    std::string service_id;
};

}
