#ifndef REDIS_VALUE_HEADER
#define REDIS_VALUE_HEADER

#include <variant>
#include <string>
#include <vector>

#include "redis/config.hpp"

namespace redis {
	class value {
	public:
		struct ErrorTag {};

		REDIS_CLIENT_DECL value();
		REDIS_CLIENT_DECL value(value &&other);
		REDIS_CLIENT_DECL value(int64_t i);
		REDIS_CLIENT_DECL value(const char *s);
		REDIS_CLIENT_DECL value(const std::string &s);
		REDIS_CLIENT_DECL value(std::vector<char> buf);
		REDIS_CLIENT_DECL value(std::vector<char> buf, struct ErrorTag);
		REDIS_CLIENT_DECL value(std::vector<value> array);

		value(const value &) = default;
		value& operator = (const value &) = default;
		value& operator = (value &&) = default;

		// Return the _value as a std::string if
		// type is a byte string; otherwise returns an empty std::string.
		REDIS_CLIENT_DECL std::string toString() const;

		// Return the _value as a std::vector<char> if
		// type is a byte string; otherwise returns an empty std::vector<char>.
		REDIS_CLIENT_DECL std::vector<char> toByteArray() const;

		// Return the _value as a std::vector<value> if
		// type is an int; otherwise returns 0.
		REDIS_CLIENT_DECL int64_t toInt() const;

		// Return the _value as an array if type is an array;
		// otherwise returns an empty array.
		REDIS_CLIENT_DECL std::vector<value> toArray() const;

		// Return the string representation of the _value. Use
		// for dump content of the _value.
		REDIS_CLIENT_DECL std::string inspect() const;

		// Return true if _value not a error
		REDIS_CLIENT_DECL bool isOk() const;
		// Return true if _value is a error
		REDIS_CLIENT_DECL bool isError() const;

		// Return true if this is a null.
		REDIS_CLIENT_DECL bool isNull() const;
		// Return true if type is an int
		REDIS_CLIENT_DECL bool isInt() const;
		// Return true if type is an array
		REDIS_CLIENT_DECL bool isArray() const;
		// Return true if type is a string/byte array. Alias for isString();
		REDIS_CLIENT_DECL bool isByteArray() const;
		// Return true if type is a string/byte array. Alias for isByteArray().
		REDIS_CLIENT_DECL bool isString() const;

		REDIS_CLIENT_DECL bool operator == (const value &rhs) const;
		REDIS_CLIENT_DECL bool operator != (const value &rhs) const;

	protected:
		template<typename T>
		 T castTo() const;

		template<typename T>
		bool typeEq() const;

	private:
		struct NullTag {
			inline bool operator == (const NullTag &) const {
				return true;
			}
		};

		bool _error;
		std::variant<NullTag, int64_t, std::vector<char>, std::vector<value>> _value;
	};

	template<typename T>
	T value::castTo() const {
		if (auto pval = std::get_if<T>(&_value)) return *pval;
		else return T();
	}

	template<typename T>
	bool value::typeEq() const {
		if( std::holds_alternative<T>(_value)) return true;
		else return false;
	}
}

#ifdef REDIS_CLIENT_HEADER_ONLY
	#include "src/value.cpp"
#endif

#endif // REDISCLIENT_REDISVALUE_H
