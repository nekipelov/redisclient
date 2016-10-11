/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISPARSER_CPP
#define REDISCLIENT_REDISPARSER_CPP

#include <sstream>
#include <assert.h>

#include "../redisparser.h"

namespace redisclient {

RedisParser::RedisParser()
    : state(Start), bulkSize(0)
{
}

std::pair<size_t, RedisParser::ParseResult> RedisParser::parse(const char *ptr, size_t size)
{
    if( !arrayStack.empty() )
        return RedisParser::parseArray(ptr, size);
    else
        return RedisParser::parseChunk(ptr, size);
}

std::pair<size_t, RedisParser::ParseResult> RedisParser::parseArray(const char *ptr, size_t size)
{
    assert( !arrayStack.empty() );
    assert( !valueStack.empty() );

    long int arraySize = arrayStack.top();
    std::vector<RedisValue> arrayValue = valueStack.top().toArray();

    arrayStack.pop();
    valueStack.pop();

    size_t position = 0;

    if( arrayStack.empty() == false  )
    {
        std::pair<size_t, RedisParser::ParseResult>  pair = parseArray(ptr, size);

        if( pair.second != Completed )
        {
            valueStack.push(std::move(arrayValue));
            arrayStack.push(arraySize);

            return pair;
        }
        else
        {
            arrayValue.push_back( std::move(valueStack.top()) );
            valueStack.pop();
            --arraySize;
        }

        position += pair.first;
    }

    if( position == size )
    {
        valueStack.push(std::move(arrayValue));

        if( arraySize == 0 )
        {
            return std::make_pair(position, Completed);
        }
        else
        {
            arrayStack.push(arraySize);
            return std::make_pair(position, Incompleted);
        }
    }

    long int arrayIndex = 0;

    for(; arrayIndex < arraySize; ++arrayIndex)
    {
        std::pair<size_t, RedisParser::ParseResult>  pair = parse(ptr + position, size - position);

        position += pair.first;

        if( pair.second == Error )
        {
            return std::make_pair(position, Error);
        }
        else if( pair.second == Incompleted )
        {
            arraySize -= arrayIndex;
            valueStack.push(std::move(arrayValue));
            arrayStack.push(arraySize);

            return std::make_pair(position, Incompleted);
        }
        else
        {
            assert( valueStack.empty() == false );
            arrayValue.push_back( std::move(valueStack.top()) );
            valueStack.pop();
        }
    }

    assert( arrayIndex == arraySize );

    valueStack.push(std::move(arrayValue));
    return std::make_pair(position, Completed);
}

std::pair<size_t, RedisParser::ParseResult> RedisParser::parseChunk(const char *ptr, size_t size)
{
    size_t position = 0;

    for(; position < size; ++position)
    {
        char c = ptr[position];

        switch(state)
        {
            case Start:
                buf.clear();
                switch(c)
                {
                    case stringReply:
                        state = String;
                        break;
                    case errorReply:
                        state = ErrorString;
                        break;
                    case integerReply:
                        state = Integer;
                        break;
                    case bulkReply:
                        state = BulkSize;
                        bulkSize = 0;
                        break;
                    case arrayReply:
                        state = ArraySize;
                        break;
                    default:
                        state = Start;
                        return std::make_pair(position + 1, Error);
                }
                break;
            case String:
                if( c == '\r' )
                {
                    state = StringLF;
                }
                else if( isChar(c) && !isControl(c) )
                {
                    buf.push_back(c);
                }
                else
                {
                    state = Start;
                    return std::make_pair(position + 1, Error);
                }
                break;
            case ErrorString:
                if( c == '\r' )
                {
                    state = ErrorLF;
                }
                else if( isChar(c) && !isControl(c) )
                {
                    buf.push_back(c);
                }
                else
                {
                    state = Start;
                    return std::make_pair(position + 1, Error);
                }
                break;
            case BulkSize:
                if( c == '\r' )
                {
                    if( buf.empty() )
                    {
                        state = Start;
                        return std::make_pair(position + 1, Error);
                    }
                    else
                    {
                        state = BulkSizeLF;
                    }
                }
                else if( isdigit(c) || c == '-' )
                {
                    buf.push_back(c);
                }
                else
                {
                    state = Start;
                    return std::make_pair(position + 1, Error);
                }
                break;
            case StringLF:
                if( c == '\n')
                {
                    state = Start;
                    valueStack.push(buf);
                    return std::make_pair(position + 1, Completed);
                }
                else
                {
                    state = Start;
                    return std::make_pair(position + 1, Error);
                }
                break;
            case ErrorLF:
                if( c == '\n')
                {
                    state = Start;
                    RedisValue::ErrorTag tag;
                    valueStack.push(RedisValue(buf, tag));
                    return std::make_pair(position + 1, Completed);
                }
                else
                {
                    state = Start;
                    return std::make_pair(position + 1, Error);
                }
                break;
            case BulkSizeLF:
                if( c == '\n' )
                {
                    bulkSize = bufToLong(buf.data(), buf.size());
                    buf.clear();

                    if( bulkSize == -1 )
                    {
                        state = Start;
                        valueStack.push(RedisValue());  // Nil
                        return std::make_pair(position + 1, Completed);
                    }
                    else if( bulkSize == 0 )
                    {
                        state = BulkCR;
                    }
                    else if( bulkSize < 0 )
                    {
                        state = Start;
                        return std::make_pair(position + 1, Error);
                    }
                    else
                    {
                        buf.reserve(bulkSize);

                        long int available = size - position - 1;
                        long int canRead = std::min(bulkSize, available);

                        if( canRead > 0 )
                        {
                            buf.assign(ptr + position + 1, ptr + position + canRead + 1);
                        }

                        position += canRead;

                        if( bulkSize > available )
                        {
                            bulkSize -= canRead;
                            state = Bulk;
                            return std::make_pair(position + 1, Incompleted);
                        }
                        else
                        {
                            state = BulkCR;
                        }
                    }
                }
                else
                {
                    state = Start;
                    return std::make_pair(position + 1, Error);
                }
                break;
            case Bulk: {
                assert( bulkSize > 0 );

                long int available = size - position;
                long int canRead = std::min(available, bulkSize);

                buf.insert(buf.end(), ptr + position, ptr + canRead);
                bulkSize -= canRead;
                position += canRead - 1;

                if( bulkSize > 0 )
                {
                    return std::make_pair(position + 1, Incompleted);
                }
                else
                {
                    state = BulkCR;

                    if( size == position + 1 )
                    {
                        return std::make_pair(position + 1, Incompleted);
                    }
                }
                break;
            }
            case BulkCR:
                if( c == '\r')
                {
                    state = BulkLF;
                }
                else
                {
                    state = Start;
                    return std::make_pair(position + 1, Error);
                }
                break;
            case BulkLF:
                if( c == '\n')
                {
                    state = Start;
                    valueStack.push(buf);
                    return std::make_pair(position + 1, Completed);
                }
                else
                {
                    state = Start;
                    return std::make_pair(position + 1, Error);
                }
                break;
            case ArraySize:
                if( c == '\r' )
                {
                    if( buf.empty() )
                    {
                        state = Start;
                        return std::make_pair(position + 1, Error);
                    }
                    else
                    {
                        state = ArraySizeLF;
                    }
                }
                else if( isdigit(c) || c == '-' )
                {
                    buf.push_back(c);
                }
                else
                {
                    state = Start;
                    return std::make_pair(position + 1, Error);
                }
                break;
            case ArraySizeLF:
                if( c == '\n' )
                {
                    int64_t arraySize = bufToLong(buf.data(), buf.size());

                    buf.clear();
                    std::vector<RedisValue> array;

                    if( arraySize == -1 || arraySize == 0)
                    {
                        state = Start;
                        valueStack.push(std::move(array));  // Empty array
                        return std::make_pair(position + 1, Completed);
                    }
                    else if( arraySize < 0 )
                    {
                        state = Start;
                        return std::make_pair(position + 1, Error);
                    }
                    else
                    {
                        array.reserve(arraySize);
                        arrayStack.push(arraySize);
                        valueStack.push(std::move(array));

                        state = Start;

                        if( position + 1 != size )
                        {
                            std::pair<size_t, ParseResult> parseResult = parseArray(ptr + position + 1, size - position - 1);
                            parseResult.first += position + 1;
                            return parseResult;
                        }
                        else
                        {
                            return std::make_pair(position + 1, Incompleted);
                        }
                    }
                }
                else
                {
                    state = Start;
                    return std::make_pair(position + 1, Error);
                }
                break;
            case Integer:
                if( c == '\r' )
                {
                    if( buf.empty() )
                    {
                        state = Start;
                        return std::make_pair(position + 1, Error);
                    }
                    else
                    {
                        state = IntegerLF;
                    }
                }
                else if( isdigit(c) || c == '-' )
                {
                    buf.push_back(c);
                }
                else
                {
                    state = Start;
                    return std::make_pair(position + 1, Error);
                }
                break;
            case IntegerLF:
                if( c == '\n' )
                {
                    int64_t value = bufToLong(buf.data(), buf.size());

                    buf.clear();

                    valueStack.push(value);
                    state = Start;

                    return std::make_pair(position + 1, Completed);
                }
                else
                {
                    state = Start;
                    return std::make_pair(position + 1, Error);
                }
                break;
            default:
                state = Start;
                return std::make_pair(position + 1, Error);
        }
    }

    return std::make_pair(position, Incompleted);
}

RedisValue RedisParser::result()
{
    assert( valueStack.empty() == false );

    if( valueStack.empty() == false )
    {
        RedisValue value = std::move(valueStack.top());
        valueStack.pop();

        return value;
    }
    else
    {
        return RedisValue();
    }
}

/*
 * Convert string to long. I can't use atol/strtol because it
 * work only with null terminated string. I can use temporary
 * std::string object but that is slower then bufToLong.
 */
long int RedisParser::bufToLong(const char *str, size_t size)
{
    long int value = 0;
    bool sign = false;

    if( str == nullptr || size == 0 )
    {
        return 0;
    }

    if( *str == '-' )
    {
        sign = true;
        ++str;
        --size;

        if( size == 0 ) {
            return 0;
        }
    }

    for(const char *end = str + size; str != end; ++str)
    {
        char c = *str;

        // char must be valid, already checked in the parser
        assert(c >= '0' && c <= '9');

        value = value * 10;
        value += c - '0';
    }

    return sign ? -value : value;
}

}

#endif // REDISCLIENT_REDISPARSER_CPP
