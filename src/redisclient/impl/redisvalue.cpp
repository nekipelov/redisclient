/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISVALUE_CPP
#define REDISCLIENT_REDISVALUE_CPP

#include <string.h>
#include <boost/lexical_cast.hpp>
#include "../redisvalue.h"

RedisValue::RedisValue()
    : value(NullTag()), error(false)
{
}

RedisValue::RedisValue(int i)
    : value(i), error(false)
{
}

RedisValue::RedisValue(const char *s)
    : value( std::vector<char>(s, s + strlen(s)) ), error(false)
{
}

RedisValue::RedisValue(const std::string &s)
    : value( std::vector<char>(s.begin(), s.end()) ), error(false)
{
}

RedisValue::RedisValue(const std::vector<char> &buf)
    : value(buf), error(false)
{
}

RedisValue::RedisValue(const std::vector<char> &buf, struct ErrorTag &)
    : value(buf), error(true)
{
}

RedisValue::RedisValue(const std::vector<RedisValue> &array)
    : value(array), error(false)
{
}

std::vector<RedisValue> RedisValue::toArray() const
{
    return castTo< std::vector<RedisValue> >();
}

std::string RedisValue::toString() const
{
    const std::vector<char> &buf = toByteArray();
    return std::string(buf.begin(), buf.end());
}

std::vector<char> RedisValue::toByteArray() const
{
    return castTo<std::vector<char> >();
}

int RedisValue::toInt() const
{
    return castTo<int>();
}

std::string RedisValue::inspect() const
{
    if( isError() )
    {
        static std::string err = "error: ";
        std::string result;

        result = err;
        result += toString();

        return result;
    }
    else if( isNull() )
    {
        static std::string null = "(null)";
        return null;
    }
    else if( isInt() )
    {
        return boost::lexical_cast<std::string>(toInt());
    }
    else if( isString() )
    {
        return toString();
    }
    else
    {
        std::vector<RedisValue> values = toArray();
        std::string result = "[";

        if( values.empty() == false )
        {
            for(size_t i = 0; i < values.size(); ++i)
            {
                result += values[i].inspect();
                result += ", ";
            }

            result.resize(result.size() - 1);
            result[result.size() - 1] = ']';
        }
        else
        {
            result += ']';
        }

        return result;
    }
}

bool RedisValue::isOk() const
{
    return !isError();
}

bool RedisValue::isError() const
{
    return error;
}

bool RedisValue::isNull() const
{
    return typeEq<NullTag>();
}

bool RedisValue::isInt() const
{
    return typeEq<int>();
}

bool RedisValue::isString() const
{
    return typeEq<std::vector<char> >();
}

bool RedisValue::isByteArray() const
{
    return typeEq<std::vector<char> >();
}

bool RedisValue::isArray() const
{
    return typeEq< std::vector<RedisValue> >();
}

bool RedisValue::operator == (const RedisValue &rhs) const
{
    return value == rhs.value;
}

bool RedisValue::operator != (const RedisValue &rhs) const
{
    return !(value == rhs.value);
}

#endif // REDISCLIENT_REDISVALUE_CPP
