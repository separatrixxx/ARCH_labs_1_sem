#include "jwt.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace profi::jwt {

namespace {

std::string Base64UrlEncode(const unsigned char* data, size_t len) {
    static const char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve((len + 2) / 3 * 4);
    for (size_t i = 0; i < len; i += 3) {
        unsigned char b0 = data[i];
        unsigned char b1 = (i + 1 < len) ? data[i + 1] : 0;
        unsigned char b2 = (i + 2 < len) ? data[i + 2] : 0;
        result += kAlphabet[(b0 >> 2) & 0x3F];
        result += kAlphabet[((b0 & 0x3) << 4) | ((b1 >> 4) & 0xF)];
        result += (i + 1 < len) ? kAlphabet[((b1 & 0xF) << 2) | ((b2 >> 6) & 0x3)] : '=';
        result += (i + 2 < len) ? kAlphabet[b2 & 0x3F] : '=';
    }
    for (char& c : result) {
        if (c == '+') { c = '-'; }
        if (c == '/') { c = '_'; }
    }

    while (!result.empty() && result.back() == '=') {
        result.pop_back();
    }

    return result;
}

std::string Base64UrlEncodeStr(const std::string& s) {
    return Base64UrlEncode(reinterpret_cast<const unsigned char*>(s.data()), s.size());
}

std::string Base64UrlDecode(const std::string& input) {
    std::string s = input;
    for (char& c : s) {
        if (c == '-') { c = '+'; }
        if (c == '_') { c = '/'; }
    }
    while (s.size() % 4 != 0) s += '=';
    std::string result;
    result.reserve(s.size() * 3 / 4);
    static const int kDecTable[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    for (size_t i = 0; i < s.size(); i += 4) {
        int v0 = kDecTable[(unsigned char)s[i]];
        int v1 = kDecTable[(unsigned char)s[i+1]];
        int v2 = (s[i+2] == '=') ? 0 : kDecTable[(unsigned char)s[i+2]];
        int v3 = (s[i+3] == '=') ? 0 : kDecTable[(unsigned char)s[i+3]];
        if (v0 < 0 || v1 < 0) { throw std::runtime_error("Invalid base64"); }

        result += static_cast<char>((v0 << 2) | (v1 >> 4));

        if (s[i+2] != '=') { result += static_cast<char>(((v1 & 0xF) << 4) | (v2 >> 2)); }
        if (s[i+3] != '=') { result += static_cast<char>(((v2 & 0x3) << 6) | v3); }
    }
    return result;
}

std::string HmacSha256(const std::string& data, const std::string& key) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    HMAC(EVP_sha256(),
         key.data(), static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()),
         data.size(),
         digest, &digest_len);
    return std::string(reinterpret_cast<const char*>(digest), digest_len);
}

}  // namespace

std::string Create(const std::string& user_id, const std::string& login,
                   const std::string& secret) {
    const std::string header_b64 =
        Base64UrlEncodeStr(R"({"alg":"HS256","typ":"JWT"})");

    namespace fj = userver::formats::json;
    fj::ValueBuilder payload;
    payload["sub"] = user_id;
    payload["login"] = login;
    payload["iat"] = static_cast<int64_t>(std::time(nullptr));
    payload["exp"] = static_cast<int64_t>(std::time(nullptr)) + 3600 * 24;
    const std::string payload_b64 = Base64UrlEncodeStr(fj::ToString(payload.ExtractValue()));

    const std::string signing_input = header_b64 + "." + payload_b64;
    const std::string sig = HmacSha256(signing_input, secret);
    return signing_input + "." +
           Base64UrlEncode(reinterpret_cast<const unsigned char*>(sig.data()), sig.size());
}

Claims Validate(const std::string& token, const std::string& secret) {
    const size_t dot1 = token.find('.');
    if (dot1 == std::string::npos) {
        throw std::runtime_error("Invalid JWT: missing first dot");
    }

    const size_t dot2 = token.find('.', dot1 + 1);
    if (dot2 == std::string::npos) {
        throw std::runtime_error("Invalid JWT: missing second dot");
    }

    const std::string header_payload = token.substr(0, dot2);
    const std::string expected_sig = HmacSha256(header_payload, secret);
    const std::string expected_sig_b64 =
        Base64UrlEncode(reinterpret_cast<const unsigned char*>(expected_sig.data()),
                        expected_sig.size());

    if (token.substr(dot2 + 1) != expected_sig_b64) {
        throw std::runtime_error("JWT signature mismatch");
    }

    namespace fj = userver::formats::json;
    const auto payload = fj::FromString(
        Base64UrlDecode(token.substr(dot1 + 1, dot2 - dot1 - 1)));

    const auto exp = payload["exp"].As<int64_t>(0);
    if (exp > 0 && std::time(nullptr) > exp) {
        throw std::runtime_error("JWT expired");
    }

    Claims claims;
    claims.user_id = payload["sub"].As<std::string>("");
    claims.login = payload["login"].As<std::string>("");

    if (claims.user_id.empty()) {
        throw std::runtime_error("JWT missing sub claim");
    }

    return claims;
}

}  // namespace profi::jwt
