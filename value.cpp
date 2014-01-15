/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: BSD
 */

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <sstream>

#include <stdio.h>

#include "value.h"

static bool stringToBool(const Value &v);
static double stringToDouble(const Value &v);
static Value &fakeValueObject();

Value::Value()
{
    // TODO надо бы переделать так, чтобы это выделение памяти было не обязательным
    holder.reset( new Holder );
    holder->type = Null;
    holder->data.ptr = 0;
}

Value::Value(Type type)
{
    if( type != Null )
    {
        holder.reset( new Holder );

        switch(type)
        {
        case Bool:
            holder->type = Bool;
            holder->data.b = false;
            break;
        case Integer:
            holder->type = Int;
            holder->data.i = 0;
            break;
        case Double:
            holder->type = Double;
            holder->data.d = 0.;
            break;
        case String:
            holder->type = String;
            holder->data.string = new std::string();
            break;
        case Array:
            holder->type = Array;
            holder->data.array = new std::vector<Value>();
            break;
        case Object:
            holder->type = Object;
            holder->data.members = new std::map<std::string, Value>();
            break;
        case UserType:
        default:
            holder->type = UserType;
            holder->data.ptr = NULL;
            break;
        }
    }
}

Value::~Value()
{
}

Value::Holder::Holder()
    : type(Value::Null)
{
}

Value::Holder::~Holder()
{
    switch(type)
    {
    case String:
        delete data.string;
        break;
    case Array:
        delete data.array;
        break;
    case Object:
        delete data.members;
        break;
    case UserType:
        delete data.userType;
        break;
    default:
        break;
    }
}

Value::Value(const struct ObjectTag &)
{
    holder.reset( new Holder );
    holder->type = Object;
    holder->data.members = new std::map<std::string, Value>();
}

Value::Value(const struct ArrayTag &)
{
    holder.reset( new Holder );
    holder->type = Array;
    holder->data.array = new std::vector<Value>();
}

Value::Value(bool b)
{
    holder.reset( new Holder );
    holder->type = Bool;
    holder->data.b = b;
}

Value::Value(int i)
{
    holder.reset( new Holder );
    holder->type = Int;
    holder->data.i = i;
}

Value::Value(unsigned int i)
{
    holder.reset( new Holder );
    holder->type = Int; // FIXME must be unsigned
    holder->data.i = i;
}

Value::Value(int64_t ii)
{
    holder.reset( new Holder );
    holder->type = Int;
    holder->data.i = ii;
}

Value::Value(uint64_t ii)
{
    holder.reset( new Holder );
    holder->type = Int; // FIXME must be unsigned
    holder->data.i = ii;
}

Value::Value(double d)
{
    holder.reset( new Holder );
    holder->type = Double;
    holder->data.d = d;
}

Value::Value(const char *s)
{
    holder.reset( new Holder );
    holder->type = String;
    holder->data.string = new std::string(s);
}

Value::Value(const std::string &s)
{
        holder.reset( new Holder );
    holder->type = String;
    holder->data.string = new std::string(s);
}

Value::Type Value::type() const
{
    if( holder )
        return holder->type;
    else
        return Null;
}

bool Value::isNull() const
{
    return type() == Null;
}

bool Value::isObject() const
{
    return type() == Object;
}

bool Value::isArray() const
{
    return type() == Array;
}

bool Value::isNumeric() const
{
    switch(type())
    {
    case Bool:
    case Int:
    case Double:
        return true;
    default:
        return false;
    }
}

bool Value::toBool() const
{
    return toValue<bool>();
}

int Value::toInt() const
{
    return toValue<int>();
}

unsigned int Value::toUInt() const
{
    return toValue<unsigned int>();
}

int64_t Value::toInt64() const
{
    return toValue<int64_t>();
}

uint64_t Value::toUInt64() const
{
    return toValue<uint64_t>();
}

double Value::toDouble() const
{
    return toValue<double>();
}

std::string Value::toString() const
{
    return toValue<std::string>();
}

size_t Value::size() const
{
    if( type() == Object )
        return holder->data.members->size();
    else if( type() == Array )
        return holder->data.array->size();
    else
        return 0;
}

bool Value::isEmpty() const
{
    if( type() == Object || type() == Array )
        return size() == 0;
    else
        return type() == Null;
}

bool Value::hasMember(const std::string &name) const
{
    if( type() == Object )
        return holder->data.members->find(name) != holder->data.members->end();

    return false;
}

Value Value::member(const std::string &name)
{
    if( type() == Object )
        return (*holder->data.members)[name];
    else
        return Value();
}

const Value Value::member(const std::string &name) const
{
    if( type() == Object )
    {
        std::map<std::string, Value>::iterator it = holder->data.members->find(name);

        if( it != holder->data.members->end() )
            return it->second;
    }

    return Value();
}

Value &Value::operator[] (const std::string &memberName)
{
    if( type() == Object )
        return (*holder->data.members)[memberName];
    else
        return fakeValueObject();
}

const Value Value::operator[] (const std::string &memberName) const
{
    return member(memberName);
}

Value Value::append(const Value &value)
{
    if( type() == Array ) {
        holder->data.array->push_back(value);
        return *this;
    }

    throw Value();
}

Value Value::at(size_t arrayIndex)
{
    if( type() == Array )
    {
        if( arrayIndex >= holder->data.array->size() )
            holder->data.array->resize(arrayIndex + 1);

        return (*holder->data.array)[arrayIndex];
    }

    return Value();
}

const Value Value::at(size_t arrayIndex) const
{
    if( type() == Array )
    {
        if(  holder->data.array->size() > arrayIndex )
            return (*holder->data.array)[arrayIndex];
    }

    return Value();
}

Value &Value::operator[] (size_t index)
{
    if( type() == Array )
    {
        if( index >= holder->data.array->size() )
            holder->data.array->resize(index + 1);

        return (*holder->data.array)[index];
    }
    else
    {
        return fakeValueObject();
    }
}

const Value Value::operator[] (size_t index) const
{
    return at(index);
}

int64_t Value::safeCastToNumber(const Value &value)
{
    const boost::shared_ptr<Holder> &holder = value.holder;
    switch( holder->type )
    {
    case Value::Bool:
        return holder->data.b;
    case Value::Int:
        return holder->data.i;
    case Value::Double:
        return holder->data.d;
    case Value::String:
#ifdef _MSC_VER
        return _strtoi64(holder->data.string->c_str(), 0, 10);
#else
        return strtoll(holder->data.string->c_str(), 0, 10);
#endif
    default:
        assert( false );
        return 0;
    }
}

bool Value::convertHelper(const Value &v, Value::Type type, void *ptr)
{
    assert( v.type() != Null );
    assert( v.type() != type );
    assert( ptr );

    switch(type) {
    case Bool: {
        int64_t *b = reinterpret_cast<int64_t *>( ptr );
        switch( v.type() ) {
        case Int:
        case Double:
            *b = safeCastToNumber(v) != 0;
            return true;
        case String:
            *b = stringToBool(v);
            return true;
        default:
            *b = false;
            return false;
        }
    }
    case Double: {
        double *d = reinterpret_cast<double *>( ptr );
        switch( v.type() ) {
        case Bool:
        case Int:
            *d = safeCastToNumber(v);
            return true;
        case String:
            *d = stringToDouble(v);
            return true;
        default:
            *d = 0;
            return false;
        }
    }
    case Int:
        *reinterpret_cast<int *>( ptr ) = safeCastToNumber(v);
        return true;
    case String: {
        std::string *s = reinterpret_cast<std::string *>( ptr );
        std::stringstream ss;
        switch( v.type() ) {
        case Bool:
            if( v.holder->data.b == true )
                *s = "true";
            else
                *s = "false";
            return true;
        case Int:
            ss << safeCastToNumber(v);
            *s = ss.str();
            return true;
        case Double:
            ss << v.toDouble();
            *s = ss.str();
            return true;
        case String:
            *s = v.toString();
            return true;
        default:
            return false;
        }
    }
    default:
        return false;
    }
}

static bool stringToBool(const Value &v)
{
    const std::string &s = v.toString();

#ifdef _MSC_VER
    if( s.empty() || stricmp(s.c_str(), "0") == 0 || stricmp(s.c_str(), "false") == 0 )
#else
    if( s.empty() || strcasecmp(s.c_str(), "0") == 0 || strcasecmp(s.c_str(), "false") == 0 )
#endif
    {
        return false;
    }
    else
    {
        return true;
    }
}

static double stringToDouble(const Value &v)
{
    const std::string &s = v.toString();

    if( s.empty() == false  )
        return strtod( s.c_str(), 0 );
    else
        return 0;
}

class Value::ValueIteratorPrivate {
public:
    ValueIteratorPrivate(const Value &value)
        : value(value)
    {
    }

    const Value &value;

    std::map<std::string, Value>::const_iterator mapIterator;
    std::vector<Value>::const_iterator arrayIterator;
};

Value::ValueIterator::ValueIterator(const Value &value)
{
    pimpl.reset(new Value::ValueIteratorPrivate(value));

    if( value.type() == Value::Object )
        pimpl->mapIterator = value.holder->data.members->begin();
    else if( value.type() == Value::Array )
        pimpl->arrayIterator = value.holder->data.array->begin();
}

Value::ValueIterator::~ValueIterator()
{
}

bool Value::ValueIterator::hasNext() const
{
    if( pimpl->value.type() == Value::Object )
        return pimpl->mapIterator != pimpl->value.holder->data.members->end();
    else if( pimpl->value.type() == Value::Array )
        return pimpl->arrayIterator != pimpl->value.holder->data.array->end();
    else
        return false;
}

bool Value::ValueIterator::hasPrev() const
{
    if( pimpl->value.type() == Value::Object )
        return pimpl->mapIterator != pimpl->value.holder->data.members->begin();
    else if( pimpl->value.type() == Value::Array )
        return pimpl->arrayIterator != pimpl->value.holder->data.array->begin();
    else
        return false;
}

const Value &Value::ValueIterator::next()
{
    if( pimpl->value.type() == Value::Object ) {
        assert( pimpl->mapIterator != pimpl->value.holder->data.members->end() );
        return (pimpl->mapIterator++)->second;
    }
    else if( pimpl->value.type() == Value::Array ) {
        assert( pimpl->arrayIterator != pimpl->value.holder->data.array->end() );
        return *pimpl->arrayIterator++;
    }
    else {
        assert( false );
    }
}

const Value &Value::ValueIterator::prev()
{
    if( pimpl->value.type() == Value::Object ) {
        assert( pimpl->mapIterator != pimpl->value.holder->data.members->begin() );
        return (pimpl->mapIterator--)->second;
    }
    else if( pimpl->value.type() == Value::Array ) {
        assert( pimpl->arrayIterator != pimpl->value.holder->data.array->begin() );
        return *pimpl->arrayIterator--;
    }
    else {
        assert( false );
    }
}

Value Value::ValueIterator::value() const
{
    if( pimpl->value.type() == Value::Object ) {
        assert( pimpl->mapIterator != pimpl->value.holder->data.members->end() );
        return pimpl->mapIterator->second;
    }
    else if( pimpl->value.type() == Value::Array ) {
        assert( pimpl->arrayIterator != pimpl->value.holder->data.array->end() );
        return *pimpl->arrayIterator;
    }

    throw 1;
}


void Value::dump(int level) const
{
    std::string tabs;

    for(int i = 0; i < level; ++i)
        tabs += "    ";

    if( type() == Null )
    {
        fprintf(stderr, "%s variable is null\n", tabs.c_str() );
    }
    else if( type() == Array )
    {
        fprintf(stderr, "%sarray: \n", tabs.c_str());

        std::vector<Value>::iterator it = holder->data.array->begin();
        std::vector<Value>::iterator end = holder->data.array->end();

        for(; it != end; ++it)
            it->dump(level+1);
    }
    else if( type() == Object )
    {
        fprintf(stderr, "%sobject: \n", tabs.c_str());
        std::map<std::string, Value>::iterator it = holder->data.members->begin();
        std::map<std::string, Value>::iterator end = holder->data.members->end();

        for(; it != end; ++it) {
            fprintf(stderr, "%s    %s ->\n", tabs.c_str(), it->first.c_str());
            it->second.dump(level+2);
        }
    }
    else if( type() == UserType )
    {
        fprintf(stderr, "%suser type: (none)\n", tabs.c_str());
    }
    else
    {
        fprintf( stderr, "%stype: %d, value: %s\n", tabs.c_str(), type(), toString().c_str());
    }
}

Value operator + (const Value &lhs, const Value &rhs)
{
    if( lhs.type() == Value::String && rhs.type() == Value::String )
    {
        return lhs.toString() + rhs.toString();
    }
    else if( lhs.type() == Value::Array || rhs.type() == Value::Array )
    {
        Value result(Value::Array);

        std::vector<Value> *vec = result.holder->data.array;

        vec->reserve(lhs.holder->data.array->size() + rhs.holder->data.array->size());
        vec->assign(lhs.holder->data.array->begin(), lhs.holder->data.array->end());
        vec->insert(lhs.holder->data.array->end(),
                      rhs.holder->data.array->begin(), rhs.holder->data.array->end());
        return result;
    }
    else
    {
        switch( lhs.type() ) {
        case Value::Int:
            switch(rhs.type()) {
            case Value::Int:
                return lhs.toInt() + rhs.toInt();
            case Value::Double:
                return lhs.toInt() + rhs.toDouble();
            default:
                break;
            }
        case Value::Double:
            switch(rhs.type()) {
            case Value::Int:
                return lhs.toDouble() + rhs.toInt();
            case Value::Double:
                return lhs.toDouble() + rhs.toDouble();
            default:
                break;
            }
        default:
            break;
        }
    }

    // Ошибка
    return Value();
}

Value operator - (const Value &lhs, const Value &rhs)
{
    if( lhs.type() == Value::Array || rhs.type() == Value::Array )
    {
        Value result( Value::Array );

        std::vector<Value> *vec = result.holder->data.array;

        if( lhs.holder->data.array->size() > rhs.holder->data.array->size() )
            vec->reserve(lhs.holder->data.array->size() - rhs.holder->data.array->size());

        size_t size = lhs.holder->data.array->size();
        std::vector<Value>::iterator begin = rhs.holder->data.array->begin();
        std::vector<Value>::iterator end   = rhs.holder->data.array->end();

        for(size_t i = 0; i < size; ++i)
        {
            const Value &item = (*lhs.holder->data.array)[i];
            std::vector<Value>::iterator it = std::find(begin, end, item);
            if( it == end )
                vec->push_back(item);
        }

        return result;
    }
    else
    {
        switch( lhs.type() ) {
        case Value::Int:
            switch(rhs.type()) {
            case Value::Int:
                return lhs.toInt() - rhs.toInt();
            case Value::Double:
                return lhs.toInt() - rhs.toDouble();
            default:
                break;
            }
        case Value::Double:
            switch(rhs.type()) {
            case Value::Int:
                return lhs.toDouble() - rhs.toInt();
            case Value::Double:
                return lhs.toDouble() - rhs.toDouble();
            default:
                break;
            }
        default:
            break;
        }
    }

    return Value();
}

Value operator * (const Value &lhs, const Value &rhs)
{
    if( lhs.type() == Value::String && rhs.type() == Value::Int )
    {
        const std::string *source = lhs.holder->data.string;
        std::string result;
        int factor = rhs.toInt();
        int size = source->size();

        result.reserve(size * factor);

        for(int i = 0; i < factor; ++i)
            result.insert( result.end(), source->begin(), source->end() );

        return result;
    }
    else if( lhs.type() == Value::Array && rhs.type() == Value::Int )
    {
        Value result(Value::Array);
        std::vector<Value> *vec = result.holder->data.array;
        const std::vector<Value> *source = lhs.holder->data.array;
        int factor = rhs.toInt();

        vec->reserve(source->size() * factor);

        for(int i = 0; i < factor; ++i)
            vec->insert( vec->begin(), source->begin(), source->end());

        return result;
    }
    else
    {
        switch( lhs.type() ) {
        case Value::Int:
            switch(rhs.type()) {
            case Value::Int:
                return lhs.toInt() * rhs.toInt();
            case Value::Double:
                return lhs.toInt() * rhs.toDouble();
            default:
                break;
            }
        case Value::Double:
            switch(rhs.type()) {
            case Value::Int:
                return lhs.toDouble() * rhs.toInt();
            case Value::Double:
                return lhs.toDouble() * rhs.toDouble();
            default:
                break;
            }
        default:
            break;
        }
    }

    return Value();
}

Value operator / (const Value &lhs, const Value &rhs)
{
    switch( lhs.type() ) {
    case Value::Int:
        switch(rhs.type()) {
        case Value::Int:
            if( rhs.toInt() == 0 )
                return Value(); // divide by zero!
            else
                return lhs.toInt() / rhs.toInt();
        case Value::Double:
            if( rhs.toDouble() == 0 )
                return Value(); // divide by zero!
            else
                return lhs.toInt() / rhs.toDouble();
        default:
            break;
        }
    case Value::Double:
        switch(rhs.type()) {
        case Value::Int:
            if( rhs.toInt() == 0 )
                return Value(); // divide by zero!
            else
                return lhs.toDouble() / rhs.toInt();
        case Value::Double:
            if( rhs.toDouble() == 0 )
                return Value(); // divide by zero!
            else
                return lhs.toDouble() / rhs.toDouble();
        default:
            break;
        }
    default:
        break;
    }

    return Value();
}

bool operator == (const Value &lhs, const Value &rhs)
{
    if( &lhs == &rhs )
        return true;

    switch( lhs.type() ) {
    case Value::Null:
        if(rhs.type() == Value::Null)
            return true;
        else
            return false;
    case Value::Bool:
        if(rhs.type() == Value::Bool)
            return lhs.toBool() == rhs.toBool();
        else
            return false;
    case Value::Int:
        if(rhs.type() == Value::Int)
            return lhs.toInt() == rhs.toInt();
        else if(rhs.type() == Value::Double)
            return lhs.toInt() == rhs.toDouble();
        else
            return false;
    case Value::Double:
        if(rhs.type() == Value::Double)
            return lhs.toDouble() == rhs.toDouble();
        else if( rhs.type() == Value::Int)
            return lhs.toDouble() == rhs.toInt();
        else
            return false;
    case Value::String:
        if(rhs.type() == Value::String)
            return lhs.toString() == rhs.toString();
        else
            return false;
    case Value::Array:
        if(rhs.type() == Value::Bool)
            return lhs.toBool() == rhs.toBool();
        else
            return false;
    case Value::Object:
        if(rhs.type() == Value::Object)
            return lhs.holder.get() == rhs.holder.get();
        else
            return false;
    default:
        return false;
    }
}

bool operator != (const Value &lhs, const Value &rhs)
{
    return !(lhs == rhs);
}

bool operator >= (const Value &lhs, const Value &rhs)
{
    return !(lhs < rhs);
}

bool operator > (const Value &lhs, const Value &rhs)
{
    if( &lhs == &rhs )
        return false;

    if( lhs.isEmpty() )
        return !rhs.isEmpty();

    if( rhs.isEmpty() )
        return true;

    switch( lhs.type() ) {
    case Value::Bool:
        if(rhs.isNumeric() )
            return lhs.toBool() > rhs.toBool();
        else
            return false;
    case Value::Int:
        if(rhs.isNumeric())
        {
            if( rhs.type() == Value::Double )
                return lhs.toInt() > rhs.toDouble();
            else
                return lhs.toInt() > rhs.toInt();
        }
        else
        {
            return false;
        }
    case Value::Double:
        if(rhs.isNumeric())
            return lhs.toDouble() > rhs.toDouble();
        else
            return false;
    case Value::String:
        if(rhs.type() == Value::String)
            return lhs.toString() > rhs.toString();
        else
            return false;
    case Value::Array:
        return false;
    case Value::Object:
        return false;
    default:
        return false;
    }
}

bool operator <= (const Value &lhs, const Value &rhs)
{
    return !(lhs > rhs);
}

bool operator < (const Value &lhs, const Value &rhs)
{
    if( &lhs == &rhs )
        return false;

    if( lhs.isEmpty() )
        return rhs.isEmpty();

    if( rhs.isEmpty() )
        return false;

    switch( lhs.type() ) {
    case Value::Bool:
        if(rhs.isNumeric() )
            return lhs.toBool() < rhs.toBool();
        else
            return false;
    case Value::Int:
        if(rhs.isNumeric())
        {
            if( rhs.type() == Value::Double )
                return lhs.toInt() < rhs.toDouble();
            else
                return lhs.toInt() < rhs.toInt();
        }
        else
        {
            return false;
        }
    case Value::Double:
        if(rhs.isNumeric())
            return lhs.toDouble() < rhs.toDouble();
        else
            return false;
    case Value::String:
        if(rhs.type() == Value::String)
            return lhs.toString() < rhs.toString();
        else
            return false;
    case Value::Array:
        return false;
    case Value::Object:
        return false;
    default:
        return false;
    }
}

static Value &fakeValueObject()
{
    static Value value;
    value = Value();
    return value;
}
