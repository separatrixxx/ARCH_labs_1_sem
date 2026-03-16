#include "jwt.hpp"

#include <ctime>
#include <stdexcept>

#include <userver/crypto/base64.hpp>
#include <userver/crypto/signers.hpp>
#include <userver/crypto/verifiers.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace profi::jwt {

namespace {

namespace cb64 = userver::crypto::base64;

}

std::string Create(const std::string& user_id, const std::string& login,
                   const std::string& secret) {
    const std::string header =
        cb64::Base64UrlEncode(R"({"alg":"HS256","typ":"JWT"})",
                              cb64::Pad::kWithout);

    namespace fj = userver::formats::json;
    fj::ValueBuilder payload;
    payload["sub"] = user_id;
    payload["login"] = login;
    payload["iat"] = static_cast<int64_t>(std::time(nullptr));
    payload["exp"] = static_cast<int64_t>(std::time(nullptr)) + 3600 * 24;
    const std::string payload_b64 =
        cb64::Base64UrlEncode(fj::ToString(payload.ExtractValue()),
                              cb64::Pad::kWithout);

    const std::string signing_input = header + "." + payload_b64;

    userver::crypto::SignerHs256 signer(secret);
    const std::string sig = signer.Sign({signing_input});
    const std::string sig_b64 = cb64::Base64UrlEncode(sig, cb64::Pad::kWithout);

    return signing_input + "." + sig_b64;
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
    const std::string sig_b64 = token.substr(dot2 + 1);

    const std::string raw_sig = cb64::Base64UrlDecode(sig_b64);
    userver::crypto::VerifierHs256 verifier(secret);
    try {
        verifier.Verify({header_payload}, raw_sig);
    } catch (const userver::crypto::CryptoException&) {
        throw std::runtime_error("JWT signature mismatch");
    }

    namespace fj = userver::formats::json;
    const auto payload = fj::FromString(
        cb64::Base64UrlDecode(token.substr(dot1 + 1, dot2 - dot1 - 1)));

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

}
