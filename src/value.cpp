#ifndef REDIS_VALUE_SOURCE
#define REDIS_VALUE_SOURCE

#include <string.h>

#include "redis/value.hpp"

namespace redis {
	value::value() : _value(NullTag()), _error(false) {
	}

	value::value(value &&other) : _value(std::move(other._value)), _error(other._error) {
	}

	value::value(int64_t i) : _value(i), _error(false) {
	}

	value::value(const char *s) : _value( std::vector<char>(s, s + strlen(s)) ), _error(false) {
	}

	value::value(const std::string &s) : _value( std::vector<char>(s.begin(), s.end()) ), _error(false) {
	}

	value::value(std::vector<char> buf) : _value(std::move(buf)), _error(false) {
	}

	value::value(std::vector<char> buf, struct ErrorTag) : _value(std::move(buf)), _error(true) {
	}

	value::value(std::vector<value> array) : _value(std::move(array)), _error(false) {
	}

	std::vector<value> value::toArray() const {
		return castTo< std::vector<value> >();
	}

	std::string value::toString() const {
		const std::vector<char> &buf = toByteArray();
		return std::string(buf.begin(), buf.end());
	}

	std::vector<char> value::toByteArray() const {
		return castTo<std::vector<char> >();
	}

	int64_t value::toInt() const {
		return castTo<int64_t>();
	}

	std::string value::inspect() const {
		if (isError()) {
			static std::string err = "_error: ";
			std::string result;

			result = err;
			result += toString();

			return result;
		} else if(isNull()) {
			static std::string null = "(null)";
			return null;
		} else if(isInt()) {
			return std::to_string(toInt());
		} else if(isString()) {
			return toString();
		} else {
			std::vector<value> values = toArray();
			std::string result = "[";

			if(values.empty() == false) {
				for(size_t i = 0; i < values.size(); ++i) {
					result += values[i].inspect();
					result += ", ";
				}

				result.resize(result.size() - 1);
				result[result.size() - 1] = ']';
			} else {
				result += ']';
			}

			return result;
		}
	}

	bool value::isOk() const {
		return !isError();
	}

	bool value::isError() const {
		return _error;
	}

	bool value::isNull() const {
		return typeEq<NullTag>();
	}

	bool value::isInt() const {
		return typeEq<int64_t>();
	}

	bool value::isString() const {
		return typeEq<std::vector<char> >();
	}

	bool value::isByteArray() const {
		return typeEq<std::vector<char> >();
	}

	bool value::isArray() const {
		return typeEq< std::vector<value> >();
	}

	bool value::operator == (const value &rhs) const {
		return _value == rhs._value;
	}

	bool value::operator != (const value &rhs) const {
		return !(_value == rhs._value);
	}
}

#endif

