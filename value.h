/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: BSD
 */

#ifndef CPPTL_VALUE_H
#define CPPTL_VALUE_H

#include <typeinfo>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <stdint.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

class Holder;

class Value {
public:
    enum Type {
        Null = 0,

        // Base types
        Bool = 1,
        Int = 2,
        Integer = 2,    // alias for Int
        Double = 3,
        String = 4,
        Array =  5,
        Object = 6,
        UserType = 7,
        Last = 0xFFFFFFFF
    };

    struct ObjectTag {};
    struct ArrayTag  {};

    Value();
    Value(Type type);
    Value(const struct ObjectTag &);
    Value(const struct ArrayTag &);

    Value(bool b);
    Value(int i);
    Value(unsigned int i);
    Value(int64_t ii);
    Value(uint64_t ii);
    Value(double d);

    Value(const char *s);
    Value(const std::string &s);

    ~Value();

    Type type() const;
    bool isNull()  const;
    bool isObject() const;
    bool isArray() const;

    bool isNumeric() const;

    bool toBool() const;
    int toInt() const;
    unsigned int toUInt() const;
    int64_t toInt64() const;
    uint64_t toUInt64() const;
    double toDouble() const;

    std::string toString() const;

    //TODO get(index), get(name), isValidIndex(int), size
    //TODO isEmpty for NULL, empty string, empty object, empty array...
    //TODO append for array, object

    /* object and array */
    size_t size() const;
    bool isEmpty() const;

    /* object */
    bool hasMember(const std::string &name) const;
    Value member(const std::string &name);
    const Value member(const std::string &name) const;

    Value &operator[] (const std::string &propertyName);
    const Value operator[] (const std::string &propertyName) const;

    /* array */
    Value append(const Value &value);

    Value at(size_t arrayIndex);
    const Value at(size_t arrayIndex) const;

    Value &operator[] (size_t arrayIndex);
    const Value operator[] (size_t arrayIndex) const;

    class ValueIteratorPrivate;
    class ValueIterator  {
    public:
        ValueIterator(const Value &value);
        ~ValueIterator();

        bool hasNext() const;
        bool hasPrev() const;

        const Value &next();
        const Value &prev();

        Value value() const;

    private:
        boost::scoped_ptr<ValueIteratorPrivate> pimpl;
    };

    template<typename T>
    static Value fromValue(const T &value);

    template<typename T>
    T toValue() const;

    /* for debug */
    void dump(int level = 0) const;

protected:
    static int64_t safeCastToNumber(const Value &);

    static bool convertHelper(const Value &v, Value::Type type, void *ptr);

private:
    struct UserTypeHolderBase {
        virtual ~UserTypeHolderBase() {};
        virtual const std::type_info &type() const = 0;
    };

    template<typename T>
    struct UserTypeHolder : public UserTypeHolderBase {
        UserTypeHolder(const T &t) : t(t) {}

        virtual const std::type_info &type() const {
            return typeid(T);
        }

        T t;

    private:
        UserTypeHolder();
        UserTypeHolder(const Holder &);
        UserTypeHolder & operator = (const Holder &);
    };

    class Holder {
    public:
        Holder();
        ~Holder();

        union {
            bool b;
            int64_t i;
            double d;
            std::string *string;
            std::vector<Value> *array;
            std::map<std::string, Value> *members;
            void *ptr;
            UserTypeHolderBase *userType;
        } data;

        Value::Type type;
    };

    mutable boost::shared_ptr<Holder> holder;

    friend class ValueIterator;
    friend Value operator + (const Value &lhs, const Value &rhs);
    friend Value operator - (const Value &lhs, const Value &rhs);
    friend Value operator * (const Value &lhs, const Value &rhs);
    friend Value operator / (const Value &lhs, const Value &rhs);
    friend bool operator == (const Value &lhs, const Value &rhs);
    friend bool operator != (const Value &lhs, const Value &rhs);
    friend bool operator >= (const Value &lhs, const Value &rhs);
    friend bool operator > (const Value &lhs, const Value &rhs);
    friend bool operator <= (const Value &lhs, const Value &rhs);
    friend bool operator < (const Value &lhs, const Value &rhs);
};

Value operator + (const Value &lhs, const Value &rhs);
Value operator - (const Value &lhs, const Value &rhs);
Value operator * (const Value &lhs, const Value &rhs);
Value operator / (const Value &lhs, const Value &rhs);
bool operator == (const Value &lhs, const Value &rhs);
bool operator != (const Value &lhs, const Value &rhs);
bool operator >= (const Value &lhs, const Value &rhs);
bool operator > (const Value &lhs, const Value &rhs);
bool operator <= (const Value &lhs, const Value &rhs);
bool operator < (const Value &lhs, const Value &rhs);

template<typename T>
struct ValueTypeInfo {
    enum {
        id = Value::UserType
    };
    typedef T value_type;
    static Value::Type typeId() {
        return static_cast<Value::Type>(id);
    }
};

#define VALUE_DECLARE_METATYPE(TYPE,STORE_TYPE,NAME) \
    template<> \
    struct ValueTypeInfo<TYPE> { \
        enum { \
            id = Value::NAME \
        }; \
        typedef STORE_TYPE value_type; \
        static Value::Type typeId() { \
            return static_cast<Value::Type>(id); \
        } \
    };

VALUE_DECLARE_METATYPE(bool, bool, Value::Bool)
VALUE_DECLARE_METATYPE(int, int64_t, Value::Int)
VALUE_DECLARE_METATYPE(unsigned int, uint64_t, Value::Int)
VALUE_DECLARE_METATYPE(int64_t,int64_t, Value::Int)
VALUE_DECLARE_METATYPE(uint64_t, uint64_t, Value::Int)
VALUE_DECLARE_METATYPE(double, double, Value::Double)
VALUE_DECLARE_METATYPE(std::string, std::string, Value::String)

#undef VALUE_DECLARE_METATYPE

template<typename T>
T Value::toValue() const
{
    if( holder && isNull() == false )
    {
        Value::Type toType = ValueTypeInfo<T>::typeId();

        if( toType == UserType )
        {
            if( typeid(T) == holder->data.userType->type() ) {
                return static_cast<UserTypeHolder<T>*>( holder->data.userType )->t;
            }
        }
        else if( toType == type() )
        {
            switch(type())
            {
            case String:
            case Array:
            case Object:
            case UserType:
                return *reinterpret_cast<const typename ValueTypeInfo<T>::value_type *>( holder->data.ptr );
            default:
                return *reinterpret_cast<const typename ValueTypeInfo<T>::value_type *>( &holder->data );
            }
        }
        else if( type() == Array || type() == Object )
        {
            return T(); // Can`t convert
        }
        else
        {
            typename ValueTypeInfo<T>::value_type t;
            if( Value::convertHelper(*this, ValueTypeInfo<T>::typeId(), &t) )
                return t;
        }
    }

    //assert(false);
    std::cerr << "invalid type  " << type()  << " " << ValueTypeInfo<T>::typeId() << std::endl;

    return T();
}

template<>
inline bool Value::toValue<bool>() const
{
    if( holder && isNull() == false )
    {
        if( type() == Array || type() == Object || type() == UserType )
            return true;
        else if( type() == Bool )
            return holder->data.b;

        int64_t result;
        convertHelper(*this, ValueTypeInfo<bool>::typeId(), &result);

        return result != 0;
    }
    else
    {
        return false;
    }
}

#define VALUE_FROM_VALUE_HELPER(TYPE) \
    template<> \
    inline Value Value::fromValue<TYPE>(const TYPE &value) { \
        return Value(value); \
    } \

VALUE_FROM_VALUE_HELPER(Value)
VALUE_FROM_VALUE_HELPER(bool)
VALUE_FROM_VALUE_HELPER(int)
VALUE_FROM_VALUE_HELPER(unsigned int)
VALUE_FROM_VALUE_HELPER(int64_t)
VALUE_FROM_VALUE_HELPER(uint64_t)
VALUE_FROM_VALUE_HELPER(double)
VALUE_FROM_VALUE_HELPER(std::string)

#undef VALUE_FROM_VALUE_HELPER

template<>
inline Value Value::fromValue< std::vector<Value> >(const std::vector<Value> &vector) {
    Value result(Value::Array);

    result.holder->data.array->assign(vector.begin(), vector.end());

    return result;
}

template<typename T>
inline Value Value::fromValue(const T &value) {
    Value result;

    result.holder.reset( new Holder );
    result.holder->type = UserType;
    result.holder->data.userType = new UserTypeHolder<T>(value);

    return result;
}

#endif // CPPTL_VALUE_H
