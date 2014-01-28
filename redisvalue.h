/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISVALUE_H
#define REDISVALUE_H

#include <boost/variant.hpp>
#include <string>
#include <list>

class RedisValue {
public:
    struct NullTag {};

    RedisValue();
    RedisValue(int i);
    RedisValue(const std::string &s);
    RedisValue(const std::list<RedisValue> &list);

    std::list<RedisValue> toList() const;
    std::string toString() const;
    int toInt() const;

    bool isNull() const;
    bool isInt() const;
    bool isString() const;
    bool isList() const;

protected:
    template<typename T>
    T castTo() const;

    template<typename T>
    bool typeEq() const;

private:
    boost::variant<NullTag, int, std::string, std::list<RedisValue> > value;
};


template<typename T>
T RedisValue::castTo() const
{
    if( value.type() == typeid(T) )
        return boost::get<T>(value);
    else
        return T();
}

template<typename T>
bool RedisValue::typeEq() const
{
    if( value.type() == typeid(T) )
        return true;
    else
        return false;
}


#endif // VALUE_H
