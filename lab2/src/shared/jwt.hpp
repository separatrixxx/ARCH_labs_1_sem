#pragma once

#include <stdexcept>
#include <string>

namespace profi::jwt {

struct Claims {
    std::string user_id;
    std::string login;
};

std::string Create(const std::string& user_id, const std::string& login,
                   const std::string& secret);

Claims Validate(const std::string& token, const std::string& secret);

}
