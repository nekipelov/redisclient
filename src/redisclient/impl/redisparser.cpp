/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISPARSER_CPP
#define REDISCLIENT_REDISPARSER_CPP

#include <sstream>

#include "../redisparser.h"

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

    size_t i = 0;

    if( arrayStack.empty() == false  )
    {
        std::pair<size_t, RedisParser::ParseResult>  pair = parseArray(ptr, size);

        if( pair.second != Completed )
        {
            valueStack.push(arrayValue);
            arrayStack.push(arraySize);

            return pair;
        }
        else
        {
            arrayValue.push_back( valueStack.top() );
            valueStack.pop();
            --arraySize;
        }

        i += pair.first;
    }

    if( i == size )
    {
        valueStack.push(arrayValue);

        if( arraySize == 0 )
        {
            return std::make_pair(i, Completed);
        }
        else
        {
            arrayStack.push(arraySize);
            return std::make_pair(i, Incompleted);
        }
    }

    long int x = 0;

    for(; x < arraySize; ++x)
    {
        std::pair<size_t, RedisParser::ParseResult>  pair = parse(ptr + i, size - i);

        i += pair.first;

        if( pair.second == Error )
        {
            return std::make_pair(i, Error);
        }
        else if( pair.second == Incompleted )
        {
            arraySize -= x;
            valueStack.push(arrayValue);
            arrayStack.push(arraySize);

            return std::make_pair(i, Incompleted);
        }
        else
        {
            assert( valueStack.empty() == false );
            arrayValue.push_back( valueStack.top() );
            valueStack.pop();
        }
    }

    assert( x == arraySize );

    valueStack.push(arrayValue);
    return std::make_pair(i, Completed);
}

std::pair<size_t, RedisParser::ParseResult> RedisParser::parseChunk(const char *ptr, size_t size)
{
    size_t i = 0;

    for(; i < size; ++i)
    {
        char c = ptr[i];

        switch(state)
        {
            case Start:
                string.clear();
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
                        return std::make_pair(i + 1, Error);
                }
                break;
            case String:
                if( c == '\r' )
                {
                    state = StringLF;
                }
                else if( isChar(c) && !isControl(c) )
                {
                    string += c;
                }
                else
                {
                    state = Start;
                    return std::make_pair(i + 1, Error);
                }
                break;
            case ErrorString:
                if( c == '\r' )
                {
                    state = ErrorLF;
                }
                else if( isChar(c) && !isControl(c) )
                {
                    string += c;
                }
                else
                {
                    state = Start;
                    return std::make_pair(i + 1, Error);
                }
                break;
            case BulkSize:
                if( c == '\r' )
                {
                    if( string.empty() )
                    {
                        state = Start;
                        return std::make_pair(i + 1, Error);
                    }
                    else
                    {
                        state = BulkSizeLF;
                    }
                }
                else if( isdigit(c) || c == '-' )
                {
                    string += c;
                }
                else
                {
                    state = Start;
                    return std::make_pair(i + 1, Error);
                }
                break;
            case StringLF:
            case ErrorLF:
                if( c == '\n')
                {
                    state = Start;
                    valueStack.push(string);
                    return std::make_pair(i + 1, Completed);
                }
                else
                {
                    state = Start;
                    return std::make_pair(i + 1, Error);
                }
                break;
            case BulkSizeLF:
                if( c == '\n' )
                {
                    bulkSize = strtol(string.c_str(), 0, 10);
                    string.clear();

                    if( bulkSize == -1 )
                    {
                        state = Start;
                        valueStack.push(RedisValue());  // Nil
                        return std::make_pair(i + 1, Completed);
                    }
                    else if( bulkSize == 0 )
                    {
                        state = BulkCR;
                    }
                    else if( bulkSize < 0 )
                    {
                        state = Start;
                        return std::make_pair(i + 1, Error);
                    }
                    else
                    {
                        string.reserve(bulkSize);

                        long int available = size - i - 1;
                        long int canRead = std::min(bulkSize, available);

                        if( canRead > 0 )
                        {
                            string.assign(ptr + i + 1, ptr + i + canRead + 1);
                        }

                        i += canRead;

                        if( bulkSize > available )
                        {
                            bulkSize -= canRead;
                            state = Bulk;
                            return std::make_pair(i + 1, Incompleted);
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
                    return std::make_pair(i + 1, Error);
                }
                break;
            case Bulk: {
                assert( bulkSize > 0 );

                long int available = size - i;
                long int canRead = std::min(available, bulkSize);

                string.insert(string.end(), ptr + i, ptr + canRead);
                bulkSize -= canRead;
                i += canRead - 1;

                if( bulkSize > 0 )
                {
                    return std::make_pair(i + 1, Incompleted);
                }
                else
                {
                    state = BulkCR;

                    if( size == i + 1 )
                    {
                        return std::make_pair(i + 1, Incompleted);
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
                    return std::make_pair(i + 1, Error);
                }
                break;
            case BulkLF:
                if( c == '\n')
                {
                    state = Start;
                    valueStack.push(string);
                    return std::make_pair(i + 1, Completed);
                }
                else
                {
                    state = Start;
                    return std::make_pair(i + 1, Error);
                }
                break;
            case ArraySize:
                if( c == '\r' )
                {
                    if( string.empty() )
                    {
                        state = Start;
                        return std::make_pair(i + 1, Error);
                    }
                    else
                    {
                        state = ArraySizeLF;
                    }
                }
                else if( isdigit(c) || c == '-' )
                {
                    string += c;
                }
                else
                {
                    state = Start;
                    return std::make_pair(i + 1, Error);
                }
                break;
            case ArraySizeLF:
                if( c == '\n' )
                {
                    long int arraySize = strtol(string.c_str(), 0, 10);
                    string.clear();
                    std::vector<RedisValue> array;

                    if( arraySize == -1 || arraySize == 0)
                    {
                        state = Start;
                        valueStack.push(array);  // Empty array
                        return std::make_pair(i + 1, Completed);
                    }
                    else if( arraySize < 0 )
                    {
                        state = Start;
                        return std::make_pair(i + 1, Error);
                    }
                    else
                    {
                        array.reserve(arraySize);
                        arrayStack.push(arraySize);
                        valueStack.push(array);

                        state = Start;

                        if( i + 1 != size )
                        {
                            std::pair<size_t, ParseResult> result = parseArray(ptr + i + 1, size - i - 1);
                            result.first += i + 1;
                            return result;
                        }
                        else
                        {
                            return std::make_pair(i + 1, Incompleted);
                        }
                    }
                }
                else
                {
                    state = Start;
                    return std::make_pair(i + 1, Error);
                }
                break;
            case Integer:
                if( c == '\r' )
                {
                    if( string.empty() )
                    {
                        state = Start;
                        return std::make_pair(i + 1, Error);
                    }
                    else
                    {
                        state = IntegerLF;
                    }
                }
                else if( isdigit(c) || c == '-' )
                {
                    string += c;
                }
                else
                {
                    state = Start;
                    return std::make_pair(i + 1, Error);
                }
                break;
            case IntegerLF:
                if( c == '\n' )
                {
                    long int value = strtol(string.c_str(), 0, 10);

                    string.clear();

                    valueStack.push(value);
                    state = Start;

                    return std::make_pair(i + 1, Completed);
                }
                else
                {
                    state = Start;
                    return std::make_pair(i + 1, Error);
                }
                break;
            default:
                state = Start;
                return std::make_pair(i + 1, Error);
        }
    }

    return std::make_pair(i, Incompleted);
}

RedisValue RedisParser::result()
{
    assert( valueStack.empty() == false );

    if( valueStack.empty() == false )
    {
        RedisValue value = valueStack.top();
        valueStack.pop();

        return value;
    }
    else
    {
        return RedisValue();
    }
}

#endif // REDISCLIENT_REDISPARSER_CPP
