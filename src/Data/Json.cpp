#include "Data/Json.h"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <utility>

namespace ember::data::json {

bool Value::is_null() const noexcept { return std::holds_alternative<std::nullptr_t>(storage); }
const bool* Value::as_bool() const noexcept { return std::get_if<bool>(&storage); }
const double* Value::as_number() const noexcept { return std::get_if<double>(&storage); }
const std::string* Value::as_string() const noexcept { return std::get_if<std::string>(&storage); }
const Value::Array* Value::as_array() const noexcept { return std::get_if<Array>(&storage); }
const Value::Object* Value::as_object() const noexcept { return std::get_if<Object>(&storage); }

const Value* Value::find(std::string_view key) const noexcept {
    const auto* object = as_object();
    if (object == nullptr) return nullptr;
    const auto it = object->find(key);
    return it == object->end() ? nullptr : &it->second;
}

namespace {

constexpr unsigned kMaxDepth = 64;

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    bool run(Value& value, ParseError& error) {
        if (text_.size() > kMaxInputBytes) return fail(error, "JSON input exceeds 4 MiB limit");
        if (!valid_utf8(text_)) return fail(error, "JSON input must be valid UTF-8");
        skip_space();
        Value candidate;
        if (!parse_value(candidate, 0)) return publish_error(error);
        skip_space();
        if (!at_end()) return fail(error, "unexpected characters after JSON value");
        value = std::move(candidate);
        error = {};
        return true;
    }

private:
    static bool valid_utf8(std::string_view input) noexcept {
        std::size_t i = 0;
        while (i < input.size()) {
            const auto lead = static_cast<unsigned char>(input[i]);
            if (lead <= 0x7FU) { ++i; continue; }
            std::size_t count = 0;
            unsigned char second_min = 0x80U;
            unsigned char second_max = 0xBFU;
            if (lead >= 0xC2U && lead <= 0xDFU) count = 2;
            else if (lead >= 0xE0U && lead <= 0xEFU) {
                count = 3;
                if (lead == 0xE0U) second_min = 0xA0U;
                if (lead == 0xEDU) second_max = 0x9FU;
            } else if (lead >= 0xF0U && lead <= 0xF4U) {
                count = 4;
                if (lead == 0xF0U) second_min = 0x90U;
                if (lead == 0xF4U) second_max = 0x8FU;
            } else return false;
            if (i + count > input.size()) return false;
            const auto second = static_cast<unsigned char>(input[i + 1U]);
            if (second < second_min || second > second_max) return false;
            for (std::size_t j = 2; j < count; ++j) {
                const auto continuation = static_cast<unsigned char>(input[i + j]);
                if (continuation < 0x80U || continuation > 0xBFU) return false;
            }
            i += count;
        }
        return true;
    }
    bool at_end() const noexcept { return pos_ >= text_.size(); }
    char peek() const noexcept { return at_end() ? '\0' : text_[pos_]; }
    char take() noexcept {
        if (at_end()) return '\0';
        const char c = text_[pos_++];
        if (c == '\n') { ++line_; column_ = 1; }
        else { ++column_; }
        return c;
    }
    void skip_space() noexcept {
        while (!at_end()) {
            const char c = peek();
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break;
            take();
        }
    }
    bool set_error(std::string message) {
        if (error_message_.empty()) {
            error_message_ = std::move(message);
            error_line_ = line_;
            error_column_ = column_;
        }
        return false;
    }
    bool publish_error(ParseError& error) const {
        error.message = error_message_.empty() ? "invalid JSON" : error_message_;
        error.line = error_line_;
        error.column = error_column_;
        return false;
    }
    bool fail(ParseError& error, std::string message) {
        set_error(std::move(message));
        return publish_error(error);
    }
    bool expect(char expected) {
        if (take() == expected) return true;
        return set_error(std::string("expected '") + expected + "'");
    }
    bool parse_value(Value& out, unsigned depth) {
        if (depth > kMaxDepth) return set_error("maximum JSON nesting depth exceeded");
        skip_space();
        switch (peek()) {
        case '{': return parse_object(out, depth + 1);
        case '[': return parse_array(out, depth + 1);
        case '"': {
            std::string value;
            if (!parse_string(value)) return false;
            out.storage = std::move(value);
            return true;
        }
        case 't': return parse_literal("true", Value{true}, out);
        case 'f': return parse_literal("false", Value{false}, out);
        case 'n': return parse_literal("null", Value{nullptr}, out);
        default:
            if (peek() == '-' || (peek() >= '0' && peek() <= '9')) return parse_number(out);
            return set_error(at_end() ? "unexpected end of input" : "expected JSON value");
        }
    }
    bool parse_literal(std::string_view literal, Value value, Value& out) {
        for (const char expected : literal) {
            if (take() != expected) return set_error("invalid JSON literal");
        }
        out = std::move(value);
        return true;
    }
    static void append_utf8(std::string& output, std::uint32_t codepoint) {
        if (codepoint <= 0x7FU) output.push_back(static_cast<char>(codepoint));
        else if (codepoint <= 0x7FFU) {
            output.push_back(static_cast<char>(0xC0U | (codepoint >> 6U)));
            output.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        } else if (codepoint <= 0xFFFFU) {
            output.push_back(static_cast<char>(0xE0U | (codepoint >> 12U)));
            output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        } else {
            output.push_back(static_cast<char>(0xF0U | (codepoint >> 18U)));
            output.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        }
    }
    bool read_hex4(std::uint32_t& value) {
        value = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = take();
            unsigned digit = 0;
            if (c >= '0' && c <= '9') digit = static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f') digit = static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') digit = static_cast<unsigned>(c - 'A' + 10);
            else return set_error("invalid Unicode escape");
            value = (value << 4U) | digit;
        }
        return true;
    }
    bool parse_string(std::string& out) {
        if (!expect('"')) return false;
        while (!at_end()) {
            const unsigned char c = static_cast<unsigned char>(take());
            if (c == '"') return true;
            if (c < 0x20U) return set_error("unescaped control character in string");
            if (c != '\\') { out.push_back(static_cast<char>(c)); continue; }
            const char escape = take();
            switch (escape) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                std::uint32_t cp = 0;
                if (!read_hex4(cp)) return false;
                if (cp >= 0xD800U && cp <= 0xDBFFU) {
                    if (take() != '\\' || take() != 'u') return set_error("high surrogate requires a low surrogate");
                    std::uint32_t low = 0;
                    if (!read_hex4(low)) return false;
                    if (low < 0xDC00U || low > 0xDFFFU) return set_error("invalid low surrogate");
                    cp = 0x10000U + ((cp - 0xD800U) << 10U) + (low - 0xDC00U);
                } else if (cp >= 0xDC00U && cp <= 0xDFFFU) {
                    return set_error("unexpected low surrogate");
                }
                append_utf8(out, cp);
                break;
            }
            default: return set_error("invalid string escape");
            }
        }
        return set_error("unterminated JSON string");
    }
    bool parse_number(Value& out) {
        const std::size_t start = pos_;
        if (peek() == '-') take();
        if (peek() == '0') take();
        else {
            if (peek() < '1' || peek() > '9') return set_error("invalid number");
            while (peek() >= '0' && peek() <= '9') take();
        }
        if (peek() == '.') {
            take();
            if (peek() < '0' || peek() > '9') return set_error("fraction requires at least one digit");
            while (peek() >= '0' && peek() <= '9') take();
        }
        if (peek() == 'e' || peek() == 'E') {
            take();
            if (peek() == '+' || peek() == '-') take();
            if (peek() < '0' || peek() > '9') return set_error("exponent requires at least one digit");
            while (peek() >= '0' && peek() <= '9') take();
        }
        const std::string_view token = text_.substr(start, pos_ - start);
        double number = 0.0;
        const auto result = std::from_chars(token.data(), token.data() + token.size(),
                                            number, std::chars_format::general);
        if (result.ec != std::errc{} || result.ptr != token.data() + token.size() ||
            !std::isfinite(number))
            return set_error("number is outside the supported finite range");
        out.storage = number;
        return true;
    }
    bool parse_array(Value& out, unsigned depth) {
        take();
        Value::Array values;
        skip_space();
        if (peek() == ']') { take(); out.storage = std::move(values); return true; }
        while (true) {
            Value value;
            if (!parse_value(value, depth)) return false;
            values.push_back(std::move(value));
            skip_space();
            const char separator = take();
            if (separator == ']') break;
            if (separator != ',') return set_error("expected ',' or ']' in array");
            skip_space();
            if (peek() == ']') return set_error("trailing comma in array");
        }
        out.storage = std::move(values);
        return true;
    }
    bool parse_object(Value& out, unsigned depth) {
        take();
        Value::Object values;
        skip_space();
        if (peek() == '}') { take(); out.storage = std::move(values); return true; }
        while (true) {
            if (peek() != '"') return set_error("object key must be a string");
            std::string key;
            if (!parse_string(key)) return false;
            skip_space();
            if (!expect(':')) return false;
            Value value;
            if (!parse_value(value, depth)) return false;
            if (!values.emplace(std::move(key), std::move(value)).second)
                return set_error("duplicate object key");
            skip_space();
            const char separator = take();
            if (separator == '}') break;
            if (separator != ',') return set_error("expected ',' or '}' in object");
            skip_space();
            if (peek() == '}') return set_error("trailing comma in object");
        }
        out.storage = std::move(values);
        return true;
    }

    std::string_view text_;
    std::size_t pos_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
    std::string error_message_;
    std::size_t error_line_ = 1;
    std::size_t error_column_ = 1;
};

} // namespace

bool parse(std::string_view source, Value& destination, ParseError& error) {
    return Parser(source).run(destination, error);
}

} // namespace ember::data::json
