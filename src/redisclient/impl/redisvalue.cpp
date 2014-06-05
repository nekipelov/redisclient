/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISVALUE_CPP
#define REDISCLIENT_REDISVALUE_CPP

#include <boost/lexical_cast.hpp>
#include "../redisvalue.h"

RedisValue::RedisValue()
    : value(NullTag())
{
}

RedisValue::RedisValue(int i)
    : value(i)
{
}

RedisValue::RedisValue(const char *s)
    : value( std::string(s) )
{
}

RedisValue::RedisValue(const std::string &s)
    : value(s)
{
}

RedisValue::RedisValue(const std::vector<RedisValue> &array)
    : value(array)
{
}

std::vector<RedisValue> RedisValue::toArray() const
{
    return castTo< std::vector<RedisValue> >();
}

std::string RedisValue::toString() const
{
    return castTo<std::string>();
}

int RedisValue::toInt() const
{
    return castTo<int>();
}

std::string RedisValue::inspect() const
{
    if( isNull() )
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
    return typeEq<std::string>();
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
