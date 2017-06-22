#include "redisclient.h"
/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISASYNCCLIENT_REDISASYNCCLIENT_CPP
#define REDISASYNCCLIENT_REDISASYNCCLIENT_CPP

#include <memory>
#include <functional>



namespace redisclient {

RedisAsyncClient::RedisAsyncClient(boost::asio::io_service &ioService)
    : pimpl(std::make_shared<RedisClientImpl>(ioService))
{
    pimpl->errorHandler = std::bind(&RedisClientImpl::defaulErrorHandler, std::placeholders::_1);
}

RedisAsyncClient::~RedisAsyncClient()
{
    pimpl->close();
}

void RedisAsyncClient::connect(const boost::asio::ip::address &address,
                               unsigned short port,
                               std::function<void(bool, const std::string &)> handler)
{
    boost::asio::ip::tcp::endpoint endpoint(address, port);
    connect(endpoint, std::move(handler));
}

void RedisAsyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint,
                               std::function<void(bool, const std::string &)> handler)
{
    if( pimpl->state == State::Unconnected || pimpl->state == State::Closed )
    {
        pimpl->state = State::Connecting;
        pimpl->socket.async_connect(endpoint, std::bind(&RedisClientImpl::handleAsyncConnect,
                    pimpl, std::placeholders::_1, std::move(handler)));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::connect called on socket with state " << to_string(pimpl->state);
        handler(false, ss.str());
    }
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

void RedisAsyncClient::connect(const boost::asio::local::stream_protocol::endpoint &endpoint,
                               std::function<void(bool, const std::string &)> handler)
{
    if( pimpl->state == State::Unconnected || pimpl->state == State::Closed )
    {
        pimpl->state = State::Connecting;
        pimpl->socket.async_connect(endpoint, std::bind(&RedisClientImpl::handleAsyncConnect,
                    pimpl, std::placeholders::_1, std::move(handler)));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::connect called on socket with state " << to_string(pimpl->state);
        handler(false, ss.str());
    }
}

#endif

bool RedisAsyncClient::isConnected() const
{
    return pimpl->getState() == State::Connected ||
            pimpl->getState() == State::Subscribed;
}

void RedisAsyncClient::disconnect()
{
    pimpl->close();
}

void RedisAsyncClient::installErrorHandler(std::function<void(const std::string &)> handler)
{
    pimpl->errorHandler = std::move(handler);
}

void RedisAsyncClient::command(const std::string &cmd, std::deque<RedisBuffer> args,
                          std::function<void(RedisValue)> handler)
{
    if(stateValid())
    {
        args.emplace_front(cmd);

        pimpl->post(std::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    std::move(pimpl->makeCommand(args)), std::move(handler)));
    }
}

RedisAsyncClient::Handle RedisAsyncClient::subscribe(
        const std::string &channel,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handler)
{
    auto handleId = pimpl->subscribe("subscribe", channel, msgHandler, handler);    
    return { handleId , channel };
}

RedisAsyncClient::Handle RedisAsyncClient::psubscribe(
    const std::string &pattern,
    std::function<void(std::vector<char> msg)> msgHandler,
    std::function<void(RedisValue)> handler)
{
    auto handleId = pimpl->subscribe("psubscribe", pattern, msgHandler, handler);
    return{ handleId , pattern };
}

void RedisAsyncClient::unsubscribe(const Handle &handle)
{
    pimpl->unsubscribe("unsubscribe", handle.id, handle.channel, dummyHandler);
}

void RedisAsyncClient::punsubscribe(const Handle &handle)
{
    pimpl->unsubscribe("punsubscribe", handle.id, handle.channel, dummyHandler);
}

void RedisAsyncClient::singleShotSubscribe(const std::string &channel,
                                           std::function<void(std::vector<char> msg)> msgHandler,
                                           std::function<void(RedisValue)> handler)
{
    pimpl->singleShotSubscribe("subscribe", channel, msgHandler, handler);
}

void RedisAsyncClient::singleShotPSubscribe(const std::string &pattern,
    std::function<void(std::vector<char> msg)> msgHandler,
    std::function<void(RedisValue)> handler)
{
    pimpl->singleShotSubscribe("psubscribe", pattern, msgHandler, handler);
}

void RedisAsyncClient::publish(const std::string &channel, const RedisBuffer &msg,
                          std::function<void(RedisValue)> handler)
{
    assert( pimpl->state == State::Connected );

    static const std::string publishStr = "PUBLISH";

    if( pimpl->state == State::Connected )
    {
        std::deque<RedisBuffer> items(3);

        items[0] = publishStr;
        items[1] = channel;
        items[2] = msg;

        pimpl->post(std::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), std::move(handler)));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << to_string(pimpl->state);

        pimpl->errorHandler(ss.str());
    }
}

RedisAsyncClient::State RedisAsyncClient::state() const
{
    return pimpl->getState();
}

bool RedisAsyncClient::stateValid() const
{
    assert( pimpl->state == State::Connected );

    if( pimpl->state != State::Connected )
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << to_string(pimpl->state);

        pimpl->errorHandler(ss.str());
        return false;
    }

    return true;
}

}

#endif // REDISASYNCCLIENT_REDISASYNCCLIENT_CPP
/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISCLIENTIMPL_CPP
#define REDISCLIENT_REDISCLIENTIMPL_CPP

#include <boost/asio/write.hpp>

#include <algorithm>



namespace redisclient {

RedisClientImpl::RedisClientImpl(boost::asio::io_service &ioService)
    : strand(ioService), socket(ioService), subscribeSeq(0), state(State::Unconnected)
{
}

RedisClientImpl::~RedisClientImpl()
{
    close();
}

void RedisClientImpl::close() noexcept
{
    if( state != State::Closed )
    {
        boost::system::error_code ignored_ec;

        msgHandlers.clear();

        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        socket.close(ignored_ec);

        state = State::Closed;
    }
}

RedisClientImpl::State RedisClientImpl::getState() const
{
    return state;
}

void RedisClientImpl::processMessage()
{
    socket.async_read_some(boost::asio::buffer(buf),
                           std::bind(&RedisClientImpl::asyncRead,
                                       shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void RedisClientImpl::doProcessMessage(RedisValue v)
{
    if( state == State::Subscribed )
    {
        std::vector<RedisValue> result = v.toArray();
        auto resultSize = result.size();

        if( resultSize >= 3 )
        {
            const RedisValue &command   = result[0];
            const RedisValue &queueName = result[(resultSize == 3)?1:2];
            const RedisValue &value     = result[(resultSize == 3)?2:3];
            const RedisValue &pattern   = (resultSize == 4) ? result[1] : "";

            std::string cmd = command.toString();

            if( cmd == "message" || cmd == "pmessage" )
            {
                SingleShotHandlersMap::iterator it = singleShotMsgHandlers.find(queueName.toString());
                if( it != singleShotMsgHandlers.end() )
                {
                    strand.post(std::bind(it->second, value.toByteArray()));
                    singleShotMsgHandlers.erase(it);
                }

                std::pair<MsgHandlersMap::iterator, MsgHandlersMap::iterator> pair =
                        msgHandlers.equal_range(queueName.toString());
                for(MsgHandlersMap::iterator handlerIt = pair.first;
                    handlerIt != pair.second; ++handlerIt)
                {
                    strand.post(std::bind(handlerIt->second.second, value.toByteArray()));
                }
            }
            else if( handlers.empty() == false &&
                    (cmd == "subscribe" || cmd == "unsubscribe" ||
                    cmd == "psubscribe" || cmd == "punsubscribe")
                   )
            {
                handlers.front()(std::move(v));
                handlers.pop();
            }
            else
            {
                std::stringstream ss;

                ss << "[RedisClient] invalid command: "
                   << command.toString();

                errorHandler(ss.str());
                return;
            }
        }

        else
        {
            errorHandler("[RedisClient] Protocol error");
            return;
        }
    }
    else
    {
        if( handlers.empty() == false )
        {
            handlers.front()(std::move(v));
            handlers.pop();
        }
        else
        {
            std::stringstream ss;

            ss << "[RedisClient] unexpected message: "
               <<  v.inspect();

            errorHandler(ss.str());
            return;
        }
    }
}

void RedisClientImpl::asyncWrite(const boost::system::error_code &ec, size_t)
{
    dataWrited.clear();

    if( ec )
    {
        errorHandler(ec.message());
        return;
    }

    if( dataQueued.empty() == false )
    {
        std::vector<boost::asio::const_buffer> buffers(dataQueued.size());

        for(size_t i = 0; i < dataQueued.size(); ++i)
        {
            buffers[i] = boost::asio::buffer(dataQueued[i]);
        }

        std::swap(dataQueued, dataWrited);

        boost::asio::async_write(socket, buffers,
                std::bind(&RedisClientImpl::asyncWrite, shared_from_this(),
                    std::placeholders::_1, std::placeholders::_2));
    }
}

void RedisClientImpl::handleAsyncConnect(const boost::system::error_code &ec,
                                         const std::function<void(bool, const std::string &)> &handler)
{
    if( !ec )
    {
        boost::system::error_code ec2; // Ignore errors in set_option
        socket.set_option(boost::asio::ip::tcp::no_delay(true), ec2);
        state = State::Connected;
        handler(true, std::string());
        processMessage();
    }
    else
    {
        state = State::Unconnected;
        handler(false, ec.message());
    }
}

std::vector<char> RedisClientImpl::makeCommand(const std::deque<RedisBuffer> &items)
{
    static const char crlf[] = {'\r', '\n'};

    std::vector<char> result;

    append(result, '*');
    append(result, std::to_string(items.size()));
    append<>(result, crlf);

    for(const RedisBuffer &item: items)
    {
        append(result, '$');
        append(result, std::to_string(item.size()));
        append<>(result, crlf);
        append(result, item);
        append<>(result, crlf);
    }

    return result;
}

RedisValue RedisClientImpl::doSyncCommand(const std::deque<RedisBuffer> &buff)
{
    boost::system::error_code ec;


    {
        std::vector<char> data = makeCommand(buff);
        boost::asio::write(socket, boost::asio::buffer(data), boost::asio::transfer_all(), ec);
    }

    if( ec )
    {
        errorHandler(ec.message());
        return RedisValue();
    }
    else
    {
        boost::array<char, 4096> inbuff;

        for(;;)
        {
            size_t size = socket.read_some(boost::asio::buffer(inbuff));

            for(size_t pos = 0; pos < size;)
            {
                std::pair<size_t, RedisParser::ParseResult> result =
                    redisParser.parse(inbuff.data() + pos, size - pos);

                if( result.second == RedisParser::Completed )
                {
                    return redisParser.result();
                }
                else if( result.second == RedisParser::Incompleted )
                {
                    pos += result.first;
                    continue;
                }
                else
                {
                    errorHandler("[RedisClient] Parser error");
                    return RedisValue();
                }
            }
        }
    }
}

void RedisClientImpl::doAsyncCommand(std::vector<char> buff,
                                     std::function<void(RedisValue)> handler)
{
    handlers.push( std::move(handler) );
    dataQueued.push_back(std::move(buff));

    if( dataWrited.empty() )
    {
        // start transmit process
        asyncWrite(boost::system::error_code(), 0);
    }
}

void RedisClientImpl::asyncRead(const boost::system::error_code &ec, const size_t size)
{
    if( ec || size == 0 )
    {
        if( state != State::Closed )
        {
            errorHandler(ec.message());
        }
        return;
    }

    for(size_t pos = 0; pos < size;)
    {
        std::pair<size_t, RedisParser::ParseResult> result = redisParser.parse(buf.data() + pos, size - pos);

        if( result.second == RedisParser::Completed )
        {
            doProcessMessage(std::move(redisParser.result()));
        }
        else if( result.second == RedisParser::Incompleted )
        {
            processMessage();
            return;
        }
        else
        {
            errorHandler("[RedisClient] Parser error");
            return;
        }

        pos += result.first;
    }

    processMessage();
}

void RedisClientImpl::onRedisError(const RedisValue &v)
{
    errorHandler(v.toString());
}

void RedisClientImpl::defaulErrorHandler(const std::string &s)
{
    throw std::runtime_error(s);
}

void RedisClientImpl::append(std::vector<char> &vec, const RedisBuffer &buf)
{
    vec.insert(vec.end(), buf.data(), buf.data() + buf.size());
}

void RedisClientImpl::append(std::vector<char> &vec, const std::string &s)
{
    vec.insert(vec.end(), s.begin(), s.end());
}

void RedisClientImpl::append(std::vector<char> &vec, const char *s)
{
    vec.insert(vec.end(), s, s + strlen(s));
}

void RedisClientImpl::append(std::vector<char> &vec, char c)
{
    vec.resize(vec.size() + 1);
    vec[vec.size() - 1] = c;
}

size_t RedisClientImpl::subscribe(
    const std::string &command,
    const std::string &channel,
    std::function<void(std::vector<char> msg)> msgHandler,
    std::function<void(RedisValue)> handler)
{
    assert(state == State::Connected ||
           state == State::Subscribed);

    if (state == State::Connected || state == State::Subscribed)
    {
        std::deque<RedisBuffer> items{ command, channel };

        post(std::bind(&RedisClientImpl::doAsyncCommand, this, makeCommand(items), std::move(handler)));
        msgHandlers.insert(std::make_pair(channel, std::make_pair(subscribeSeq, std::move(msgHandler))));
        state = State::Subscribed;

        return subscribeSeq++;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClientImpl::subscribe called with invalid state "
            << to_string(state);

        errorHandler(ss.str());
        return 0;
    }
}

void RedisClientImpl::singleShotSubscribe(
    const std::string &command,
    const std::string &channel,
    std::function<void(std::vector<char> msg)> msgHandler,
    std::function<void(RedisValue)> handler)
{
    assert(state == State::Connected ||
           state == State::Subscribed);

    if (state == State::Connected ||
        state == State::Subscribed)
    {
        std::deque<RedisBuffer> items{ command, channel };

        post(std::bind(&RedisClientImpl::doAsyncCommand, this, makeCommand(items), std::move(handler)));
        singleShotMsgHandlers.insert(std::make_pair(channel, std::move(msgHandler)));
        state = State::Subscribed;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClientImpl::singleShotSubscribe called with invalid state "
            << to_string(state);

        errorHandler(ss.str());
    }
}

void RedisClientImpl::unsubscribe(const std::string &command, 
                                  size_t handleId, 
                                  const std::string &channel,
                                  std::function<void(RedisValue)> handler)
{
#ifdef DEBUG
    static int recursion = 0;
    assert(recursion++ == 0);
#endif

    assert(state == State::Connected ||
           state == State::Subscribed);

    if (state == State::Connected ||
        state == State::Subscribed)
    {
        // Remove subscribe-handler
        typedef RedisClientImpl::MsgHandlersMap::iterator iterator;
        std::pair<iterator, iterator> pair = msgHandlers.equal_range(channel);

        for (iterator it = pair.first; it != pair.second;)
        {
            if (it->second.first == handleId)
            {
                msgHandlers.erase(it++);
            }
            else
            {
                ++it;
            }
        }

        std::deque<RedisBuffer> items{ command, channel };

        // Unsubscribe command for Redis
        post(std::bind(&RedisClientImpl::doAsyncCommand, this,
             makeCommand(items), handler));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClientImpl::unsubscribe called with invalid state "
            << to_string(state);

#ifdef DEBUG
        --recursion;
#endif
        errorHandler(ss.str());
        return;
    }

#ifdef DEBUG
    --recursion;
#endif
}

}

#endif // REDISCLIENT_REDISCLIENTIMPL_CPP
/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISPARSER_CPP
#define REDISCLIENT_REDISPARSER_CPP

#include <sstream>
#include <assert.h>



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
/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISSYNCCLIENT_CPP
#define REDISCLIENT_REDISSYNCCLIENT_CPP

#include <memory>
#include <functional>



namespace redisclient {

RedisSyncClient::RedisSyncClient(boost::asio::io_service &ioService)
    : pimpl(std::make_shared<RedisClientImpl>(ioService))
{
    pimpl->errorHandler = std::bind(&RedisClientImpl::defaulErrorHandler, std::placeholders::_1);
}

RedisSyncClient::~RedisSyncClient()
{
    pimpl->close();
}

bool RedisSyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint,
        std::string &errmsg)
{
    boost::system::error_code ec;

    pimpl->socket.open(endpoint.protocol(), ec);

    if( !ec )
    {
        pimpl->socket.set_option(boost::asio::ip::tcp::no_delay(true), ec);

        if( !ec )
        {
            pimpl->socket.connect(endpoint, ec);
        }
    }

    if( !ec )
    {
        pimpl->state = State::Connected;
        return true;
    }
    else
    {
        errmsg = ec.message();
        return false;
    }
}

bool RedisSyncClient::connect(const boost::asio::ip::address &address,
        unsigned short port,
        std::string &errmsg)
{
    boost::asio::ip::tcp::endpoint endpoint(address, port);

    return connect(endpoint, errmsg);
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

bool RedisSyncClient::connect(const boost::asio::local::stream_protocol::endpoint &endpoint,
        std::string &errmsg)
{
    boost::system::error_code ec;

    pimpl->socket.open(endpoint.protocol(), ec);

    if( !ec )
    {
        pimpl->socket.connect(endpoint, ec);
    }

    if( !ec )
    {
        pimpl->state = State::Connected;
        return true;
    }
    else
    {
        errmsg = ec.message();
        return false;
    }
}

#endif

void RedisSyncClient::installErrorHandler(
        std::function<void(const std::string &)> handler)
{
    pimpl->errorHandler = std::move(handler);
}

RedisValue RedisSyncClient::command(const std::string &cmd, std::deque<RedisBuffer> args)
{
    if(stateValid())
    {
        args.emplace_front(cmd);

        return pimpl->doSyncCommand(args);
    }
    else
    {
        return RedisValue();
    }
}

RedisSyncClient::State RedisSyncClient::state() const
{
    return pimpl->getState();
}

bool RedisSyncClient::stateValid() const
{
    assert( state() == State::Connected );

    if( state() != State::Connected )
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << to_string(state());

        pimpl->errorHandler(ss.str());
        return false;
    }

    return true;
}

}

#endif // REDISCLIENT_REDISSYNCCLIENT_CPP
/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISVALUE_CPP
#define REDISCLIENT_REDISVALUE_CPP

#include <string.h>



namespace redisclient {

RedisValue::RedisValue()
    : value(NullTag()), error(false)
{
}

RedisValue::RedisValue(RedisValue &&other)
    : value(std::move(other.value)), error(other.error)
{
}

RedisValue::RedisValue(int64_t i)
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

RedisValue::RedisValue(std::vector<char> buf)
    : value(std::move(buf)), error(false)
{
}

RedisValue::RedisValue(std::vector<char> buf, struct ErrorTag)
    : value(std::move(buf)), error(true)
{
}

RedisValue::RedisValue(std::vector<RedisValue> array)
    : value(std::move(array)), error(false)
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

int64_t RedisValue::toInt() const
{
    return castTo<int64_t>();
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
        return std::to_string(toInt());
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
    return typeEq<int64_t>();
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

}

#endif // REDISCLIENT_REDISVALUE_CPP

