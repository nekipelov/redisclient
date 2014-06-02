/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISVALUE_H
#define REDISVALUE_H

#include <boost/variant.hpp>
#include <string>
#include <vector>

class RedisValue {
public:
    RedisValue();
    RedisValue(int i);
    RedisValue(const char *s);
    RedisValue(const std::string &s);
    RedisValue(const std::vector<RedisValue> &array);

    std::vector<RedisValue> toArray() const;
    std::string toString() const;
    int toInt() const;

    std::string inspect() const;

    bool isNull() const;
    bool isInt() const;
    bool isString() const;
    bool isArray() const;

    bool operator == (const RedisValue &rhs) const;
    bool operator != (const RedisValue &rhs) const;

protected:
    template<typename T>
    T castTo() const;

    template<typename T>
    bool typeEq() const;

private:
    struct NullTag {
        bool operator == (const NullTag &) const {
            return true;
        }
    };


    boost::variant<NullTag, int, std::string, std::vector<RedisValue> > value;
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
