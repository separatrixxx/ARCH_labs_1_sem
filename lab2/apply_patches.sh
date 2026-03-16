#!/bin/bash

set -e

ENCODING_HPP="build/_deps/userver-src/core/src/utils/encoding.hpp"
VALUE_CPP="build/_deps/userver-src/universal/src/formats/yaml/value.cpp"
SCHEMA_CPP="build/_deps/userver-src/universal/src/yaml_config/schema.cpp"

if [ ! -f "$ENCODING_HPP" ] || [ ! -f "$VALUE_CPP" ] || [ ! -f "$SCHEMA_CPP" ]; then
    echo "Error: userver sources not found. Run cmake -B build first."
    exit 1
fi

if grep -q "kSize = 384" "$ENCODING_HPP"; then
    sed -i '' 's/static constexpr size_t kSize = 384;/static constexpr size_t kSize = 512;/' "$ENCODING_HPP"
    sed -i '' 's/static constexpr size_t kAlignment = 64;/static constexpr size_t kAlignment = 128;/' "$ENCODING_HPP"
    echo "[1/3] encoding.hpp patched"
else
    echo "[1/3] encoding.hpp already patched"
fi

if grep -q "IsConvertibleChecker" "$VALUE_CPP"; then
python3 - "$VALUE_CPP" <<'PYEOF'
import sys, re
path = sys.argv[1]
src = open(path).read()

old = '''template <class T>
bool Value::IsConvertibleToArithmetic() const {
    if (IsMissing()) {
        return false;
    }

    bool ok = true;
    value_pimpl_->as<T>(IsConvertibleChecker<T>{ok});
    return ok && !IsExplicitlyTypedString(*value_pimpl_);
}

template <class T>
T Value::ValueAsArithmetic() const {
    CheckNotMissing();

    bool ok = true;
    auto res = value_pimpl_->as<T>(IsConvertibleChecker<T>{ok});
    if (!ok || IsExplicitlyTypedString(*value_pimpl_)) {
        throw TypeMismatchException(*value_pimpl_, compiler::GetTypeName<T>(), path_.ToStringView());
    }
    return res;
}'''

new = '''template <class T>
bool Value::IsConvertibleToArithmetic() const {
    if (IsMissing()) {
        return false;
    }
    if (IsExplicitlyTypedString(*value_pimpl_)) {
        return false;
    }
    try {
        value_pimpl_->template as<T>();
        return true;
    } catch (...) {
        return false;
    }
}

template <class T>
T Value::ValueAsArithmetic() const {
    CheckNotMissing();
    if (IsExplicitlyTypedString(*value_pimpl_)) {
        throw TypeMismatchException(*value_pimpl_, compiler::GetTypeName<T>(), path_.ToStringView());
    }
    try {
        return value_pimpl_->template as<T>();
    } catch (...) {
        throw TypeMismatchException(*value_pimpl_, compiler::GetTypeName<T>(), path_.ToStringView());
    }
}'''

if old in src:
    open(path, 'w').write(src.replace(old, new, 1))
    print("[2/3] value.cpp patched")
else:
    print("[2/3] value.cpp already patched or unexpected format")
PYEOF
else
    echo "[2/3] value.cpp already patched"
fi

if grep -q "if (value.IsBool())" "$SCHEMA_CPP"; then
python3 - "$SCHEMA_CPP" <<'PYEOF'
import sys
path = sys.argv[1]
src = open(path).read()

old = '''std::variant<bool, SchemaPtr>
Parse(const formats::yaml::Value& value, formats::parse::To<std::variant<bool, SchemaPtr>>) {
    if (value.IsBool()) {
        return value.As<bool>();
    } else {
        return value.As<SchemaPtr>();
    }
}'''

new = '''std::variant<bool, SchemaPtr>
Parse(const formats::yaml::Value& value, formats::parse::To<std::variant<bool, SchemaPtr>>) {
    const auto scalar = value.IsString() ? value.As<std::string>() : std::string{};
    if (value.IsBool()) {
        return value.As<bool>();
    } else if (scalar == "true") {
        return true;
    } else if (scalar == "false") {
        return false;
    } else {
        return value.As<SchemaPtr>();
    }
}'''

if old in src:
    open(path, 'w').write(src.replace(old, new, 1))
    print("[3/3] schema.cpp patched")
else:
    print("[3/3] schema.cpp already patched or unexpected format")
PYEOF
else
    echo "[3/3] schema.cpp already patched"
fi

echo "Done. Now run: cmake --build build --target user_service catalog_service order_service"
