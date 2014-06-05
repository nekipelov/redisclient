/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISVALUE_H
#define REDISCLIENT_REDISVALUE_H

#include <boost/variant.hpp>
#include <string>
#include <vector>

#include "config.h"

class RedisValue {
public:
    REDIS_CLIENT_DECL RedisValue();
    REDIS_CLIENT_DECL RedisValue(int i);
    REDIS_CLIENT_DECL RedisValue(const char *s);
    REDIS_CLIENT_DECL RedisValue(const std::string &s);
    REDIS_CLIENT_DECL RedisValue(const std::vector<RedisValue> &array);

    // Return the value as a std::string if 
    // type is a std::string; otherwise returns an empty std::string.
    REDIS_CLIENT_DECL std::string toString() const;
    
    // Return the value as a std::vector<RedisValue> if 
    // type is an int; otherwise returns 0.
    REDIS_CLIENT_DECL int toInt() const;
    
    // Return the value as an array if type is an array;
    // otherwise returns an empty array.
    REDIS_CLIENT_DECL std::vector<RedisValue> toArray() const;

    // Return the string representation of the value. Use
    // for dump content of the value.
    REDIS_CLIENT_DECL std::string inspect() const;

    // Return true if this is a null.
    REDIS_CLIENT_DECL bool isNull() const;
    // Return true if type is an int
    REDIS_CLIENT_DECL bool isInt() const;
    // Return true if type is a string
    REDIS_CLIENT_DECL bool isString() const;
    // Return true if type is an array
    REDIS_CLIENT_DECL bool isArray() const;

    REDIS_CLIENT_DECL bool operator == (const RedisValue &rhs) const;
    REDIS_CLIENT_DECL bool operator != (const RedisValue &rhs) const;

protected:
    template<typename T>
    REDIS_CLIENT_DECL T castTo() const;

    template<typename T>
    REDIS_CLIENT_DECL bool typeEq() const;

private:
    struct NullTag {
        inline bool operator == (const NullTag &) const {
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

#ifdef REDIS_CLIENT_HEADER_ONLY
#include "impl/redisvalue.cpp"
#endif

#endif // REDISCLIENT_REDISVALUE_H
