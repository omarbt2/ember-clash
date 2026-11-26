#pragma once

#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ember::data::json {

inline constexpr std::size_t kMaxInputBytes = 4U * 1024U * 1024U;

struct Value {
    using Array = std::vector<Value>;
    using Object = std::map<std::string, Value, std::less<>>;
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    Storage storage = nullptr;

    bool is_null() const noexcept;
    const bool* as_bool() const noexcept;
    const double* as_number() const noexcept;
    const std::string* as_string() const noexcept;
    const Array* as_array() const noexcept;
    const Object* as_object() const noexcept;
    const Value* find(std::string_view key) const noexcept;
};

struct ParseError {
    std::string message;
    std::size_t line = 1;
    std::size_t column = 1;
};

// Parses strict RFC 8259 JSON. The destination is unchanged on failure.
bool parse(std::string_view source, Value& destination, ParseError& error);

} // namespace ember::data::json
