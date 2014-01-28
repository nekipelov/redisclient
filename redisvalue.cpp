/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#include "redisvalue.h"

RedisValue::RedisValue()
    : value(NullTag())
{
}

RedisValue::RedisValue(int i)
    : value(i)
{
}

RedisValue::RedisValue(const std::string &s)
    : value(s)
{
}

RedisValue::RedisValue(const std::list<RedisValue> &list)
    : value(list)
{
}

std::list<RedisValue> RedisValue::toList() const
{
    return castTo< std::list<RedisValue> >();
}

std::string RedisValue::toString() const
{
    return castTo<std::string>();
}

int RedisValue::toInt() const
{
    return castTo<int>();
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

bool RedisValue::isList() const
{
    return typeEq< std::list<RedisValue> >();
}
