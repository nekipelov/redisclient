#include "redisclient.h"
/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_PIPELINE_CPP
#define REDISCLIENT_PIPELINE_CPP





namespace redisclient
{

Pipeline::Pipeline(RedisSyncClient &client)
    : client(client)
{
}

Pipeline &Pipeline::command(std::string cmd, std::deque<RedisBuffer> args)
{
    args.push_front(std::move(cmd));
    commands.push_back(std::move(args));
    return *this;
}

RedisValue Pipeline::finish()
{
    return client.pipelined(std::move(commands));
}

}

#endif // REDISCLIENT_PIPELINE_CPP
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

void RedisAsyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint,
                               std::function<void(boost::system::error_code)> handler)
{
    if( pimpl->state == State::Closed )
    {
        pimpl->redisParser = RedisParser();
        std::move(pimpl->socket);
    }

    if( pimpl->state == State::Unconnected || pimpl->state == State::Closed )
    {
        pimpl->state = State::Connecting;
        pimpl->socket.async_connect(endpoint, std::bind(&RedisClientImpl::handleAsyncConnect,
                    pimpl, std::placeholders::_1, std::move(handler)));
    }
    else
    {
        // FIXME: add correct error message
        //std::stringstream ss;
        //ss << "RedisAsyncClient::connect called on socket with state " << to_string(pimpl->state);
        //handler(false, ss.str());
        handler(boost::system::error_code());
    }
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

void RedisAsyncClient::connect(const boost::asio::local::stream_protocol::endpoint &endpoint,
                               std::function<void(boost::system::error_code)> handler)
{
    if( pimpl->state == State::Unconnected || pimpl->state == State::Closed )
    {
        pimpl->state = State::Connecting;
        pimpl->socket.async_connect(endpoint, std::bind(&RedisClientImpl::handleAsyncConnect,
                    pimpl, std::placeholders::_1, std::move(handler)));
    }
    else
    {
        // FIXME: add correct error message
        //std::stringstream ss;
        //ss << "RedisAsyncClient::connect called on socket with state " << to_string(pimpl->state);
        //handler(false, ss.str());
        handler(boost::system::error_code());
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



namespace
{
    static const char crlf[] = {'\r', '\n'};
    inline void bufferAppend(std::vector<char> &vec, const std::string &s);
    inline void bufferAppend(std::vector<char> &vec, const std::vector<char> &s);
    inline void bufferAppend(std::vector<char> &vec, const char *s);
    inline void bufferAppend(std::vector<char> &vec, char c);
    template<size_t size>
    inline void bufferAppend(std::vector<char> &vec, const char (&s)[size]);

    inline void bufferAppend(std::vector<char> &vec, const redisclient::RedisBuffer &buf)
    {
        if (buf.data.type() == typeid(std::string))
            bufferAppend(vec, boost::get<std::string>(buf.data));
        else
            bufferAppend(vec, boost::get<std::vector<char>>(buf.data));
    }

    inline void bufferAppend(std::vector<char> &vec, const std::string &s)
    {
        vec.insert(vec.end(), s.begin(), s.end());
    }

    inline void bufferAppend(std::vector<char> &vec, const std::vector<char> &s)
    {
        vec.insert(vec.end(), s.begin(), s.end());
    }

    inline void bufferAppend(std::vector<char> &vec, const char *s)
    {
        vec.insert(vec.end(), s, s + strlen(s));
    }

    inline void bufferAppend(std::vector<char> &vec, char c)
    {
        vec.resize(vec.size() + 1);
        vec[vec.size() - 1] = c;
    }

    template<size_t size>
    inline void bufferAppend(std::vector<char> &vec, const char (&s)[size])
    {
        vec.insert(vec.end(), s, s + size);
    }

    ssize_t socketReadSomeImpl(int socket, char *buffer, size_t size,
            size_t timeoutMsec)
    {
        struct timeval tv = {static_cast<time_t>(timeoutMsec / 1000),
            static_cast<__suseconds_t>((timeoutMsec % 1000) * 1000)};
        int result = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        if (result != 0)
        {
            return result;
        }

        pollfd pfd;

        pfd.fd = socket;
        pfd.events = POLLIN;

        result = ::poll(&pfd, 1, timeoutMsec);
        if (result > 0)
        {
            return recv(socket, buffer, size, MSG_DONTWAIT);
        }
        else
        {
            return result;
        }
    }

    size_t socketReadSome(int socket, boost::asio::mutable_buffer buffer,
            const boost::posix_time::time_duration &timeout,
            boost::system::error_code &ec)
    {
        size_t bytesRecv = 0;
        size_t timeoutMsec = timeout.total_milliseconds();

        for(;;)
        {
            ssize_t result = socketReadSomeImpl(socket,
                    boost::asio::buffer_cast<char *>(buffer) + bytesRecv,
                    boost::asio::buffer_size(buffer) - bytesRecv, timeoutMsec);

            if (result < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    ec = boost::system::error_code(errno,
                            boost::asio::error::get_system_category());
                    break;
                }
            }
            else if (result == 0)
            {
                // boost::asio::error::connection_reset();
                // boost::asio::error::eof
                ec = boost::asio::error::eof;
                break;
            }
            else
            {
                bytesRecv += result;
                break;
            }
        }

        return bytesRecv;
    }


    ssize_t socketWriteImpl(int socket, const char *buffer, size_t size,
            size_t timeoutMsec)
    {
        struct timeval tv = {static_cast<time_t>(timeoutMsec / 1000),
            static_cast<__suseconds_t>((timeoutMsec % 1000) * 1000)};
        int result = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        if (result != 0)
        {
            return result;
        }

        pollfd pfd;

        pfd.fd = socket;
        pfd.events = POLLOUT;

        result = ::poll(&pfd, 1, timeoutMsec);
        if (result > 0)
        {
            return send(socket, buffer, size, 0);
        }
        else
        {
            return result;
        }
    }

    size_t socketWrite(int socket, boost::asio::const_buffer buffer,
            const boost::posix_time::time_duration &timeout,
            boost::system::error_code &ec)
    {
        size_t bytesSend = 0;
        size_t timeoutMsec = timeout.total_milliseconds();

        while(bytesSend < boost::asio::buffer_size(buffer))
        {
            ssize_t result = socketWriteImpl(socket,
                    boost::asio::buffer_cast<const char *>(buffer) + bytesSend,
                    boost::asio::buffer_size(buffer) - bytesSend, timeoutMsec);

            if (result < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    ec = boost::system::error_code(errno,
                            boost::asio::error::get_system_category());
                    break;
                }
            }
            else if (result == 0)
            {
                    // boost::asio::error::connection_reset();
                    // boost::asio::error::eof
                    ec = boost::asio::error::eof;
                    break;
            }
            else
            {
                bytesSend += result;
            }
        }

        return bytesSend;
    }

    size_t socketWrite(int socket, const std::vector<boost::asio::const_buffer> &buffers,
            const boost::posix_time::time_duration &timeout,
            boost::system::error_code &ec)
    {
        size_t bytesSend = 0;
        for(const auto &buffer: buffers)
        {
            bytesSend += socketWrite(socket, buffer, timeout, ec);

            if (ec)
                break;
        }

        return bytesSend;
    }
}

namespace redisclient {

RedisClientImpl::RedisClientImpl(boost::asio::io_service &ioService_)
    : ioService(ioService_), strand(ioService), socket(ioService),
    bufSize(0),subscribeSeq(0), state(State::Unconnected)
{
}

RedisClientImpl::~RedisClientImpl()
{
    close();
}

void RedisClientImpl::close() noexcept
{
    boost::system::error_code ignored_ec;

    msgHandlers.clear();
    decltype(handlers)().swap(handlers);

    socket.cancel(ignored_ec);
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket.close(ignored_ec);

    state = State::Closed;
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
            const RedisValue &pattern   = (resultSize == 4) ? result[1] : queueName;

            std::string cmd = command.toString();

            if( cmd == "message" || cmd == "pmessage" )
            {
                SingleShotHandlersMap::iterator it = singleShotMsgHandlers.find(pattern.toString());
                if( it != singleShotMsgHandlers.end() )
                {
                    strand.post(std::bind(it->second, value.toByteArray()));
                    singleShotMsgHandlers.erase(it);
                }

                std::pair<MsgHandlersMap::iterator, MsgHandlersMap::iterator> pair =
                        msgHandlers.equal_range(pattern.toString());
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
            std::function<void(boost::system::error_code)> handler)
{
    if( !ec )
    {
        boost::system::error_code ec2; // Ignore errors in set_option
        socket.set_option(boost::asio::ip::tcp::no_delay(true), ec2);
        state = State::Connected;
        handler(ec);
        processMessage();
    }
    else
    {
        state = State::Unconnected;
        handler(ec);
    }
}

std::vector<char> RedisClientImpl::makeCommand(const std::deque<RedisBuffer> &items)
{
    std::vector<char> result;

    bufferAppend(result, '*');
    bufferAppend(result, std::to_string(items.size()));
    bufferAppend<>(result, crlf);

    for(const auto &item: items)
    {
        bufferAppend(result, '$');
        bufferAppend(result, std::to_string(item.size()));
        bufferAppend<>(result, crlf);
        bufferAppend(result, item);
        bufferAppend<>(result, crlf);
    }

    return result;
}
RedisValue RedisClientImpl::doSyncCommand(const std::deque<RedisBuffer> &command,
        const boost::posix_time::time_duration &timeout,
        boost::system::error_code &ec)
{
    std::vector<char> data = makeCommand(command);
    socketWrite(socket.native_handle(), boost::asio::buffer(data), timeout, ec);

    if( ec )
    {
        return RedisValue();
    }

    return syncReadResponse(timeout, ec);
}

RedisValue RedisClientImpl::doSyncCommand(const std::deque<std::deque<RedisBuffer>> &commands,
        const boost::posix_time::time_duration &timeout,
        boost::system::error_code &ec)
{
    std::vector<std::vector<char>> data;
    std::vector<boost::asio::const_buffer> buffers;

    data.reserve(commands.size());
    buffers.reserve(commands.size());

    for(const auto &command: commands)
    {
        data.push_back(makeCommand(command));
        buffers.push_back(boost::asio::buffer(data.back()));
    }

    socketWrite(socket.native_handle(), buffers, timeout, ec);

    if( ec )
    {
        return RedisValue();
    }

    std::vector<RedisValue> responses;

    for(size_t i = 0; i < commands.size(); ++i)
    {
        responses.push_back(syncReadResponse(timeout, ec));

        if (ec)
        {
            return RedisValue();
        }
    }

    return RedisValue(std::move(responses));
}

RedisValue RedisClientImpl::syncReadResponse(
        const boost::posix_time::time_duration &timeout,
        boost::system::error_code &ec)
{
    for(;;)
    {
        if (bufSize == 0)
        {
            bufSize = socketReadSome(socket.native_handle(),
                    boost::asio::buffer(buf), timeout, ec);

            if (ec)
                return RedisValue();
        }

        for(size_t pos = 0; pos < bufSize;)
        {
            std::pair<size_t, RedisParser::ParseResult> result =
                redisParser.parse(buf.data() + pos, bufSize - pos);

            pos += result.first;

            ::memmove(buf.data(), buf.data() + pos, bufSize - pos);
            bufSize -= pos;

            if( result.second == RedisParser::Completed )
            {
                return redisParser.result();
            }
            else if( result.second == RedisParser::Incompleted )
            {
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
        if (ec != boost::asio::error::operation_aborted)
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
            doProcessMessage(redisParser.result());
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

#ifdef DEBUG_REDIS_PARSER
#include <iostream>
#endif



namespace redisclient {

RedisParser::RedisParser()
    : bulkSize(0)
{
    buf.reserve(64);
}

std::pair<size_t, RedisParser::ParseResult> RedisParser::parse(const char *ptr, size_t size)
{
    return RedisParser::parseChunk(ptr, size);
}

std::pair<size_t, RedisParser::ParseResult> RedisParser::parseChunk(const char *ptr, size_t size)
{
    size_t position = 0;
    State state = Start;

    if (!states.empty())
    {
        state = states.top();
        states.pop();
    }

    while(position < size)
    {
        char c = ptr[position++];
#ifdef DEBUG_REDIS_PARSER
        std::cerr << "state: " << state << ", c: " << c << "\n";
#endif

        switch(state)
        {
            case StartArray:
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
                        return std::make_pair(position, Error);
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
                    std::stack<State>().swap(states);
                    return std::make_pair(position, Error);
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
                    std::stack<State>().swap(states);
                    return std::make_pair(position, Error);
                }
                break;
            case BulkSize:
                if( c == '\r' )
                {
                    if( buf.empty() )
                    {
                        std::stack<State>().swap(states);
                        return std::make_pair(position, Error);
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
                    std::stack<State>().swap(states);
                    return std::make_pair(position, Error);
                }
                break;
            case StringLF:
                if( c == '\n')
                {
                    state = Start;
                    redisValue = RedisValue(buf);
                }
                else
                {
                    std::stack<State>().swap(states);
                    return std::make_pair(position, Error);
                }
                break;
            case ErrorLF:
                if( c == '\n')
                {
                    state = Start;
                    RedisValue::ErrorTag tag;
                    redisValue = RedisValue(buf, tag);
                }
                else
                {
                    std::stack<State>().swap(states);
                    return std::make_pair(position, Error);
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
                        redisValue = RedisValue(); // Nil
                    }
                    else if( bulkSize == 0 )
                    {
                        state = BulkCR;
                    }
                    else if( bulkSize < 0 )
                    {
                        std::stack<State>().swap(states);
                        return std::make_pair(position, Error);
                    }
                    else
                    {
                        buf.reserve(bulkSize);

                        long int available = size - position;
                        long int canRead = std::min(bulkSize, available);

                        if( canRead > 0 )
                        {
                            buf.assign(ptr + position, ptr + position + canRead);
                            position += canRead;
                            bulkSize -= canRead;
                        }


                        if (bulkSize > 0)
                        {
                            state = Bulk;
                        }
                        else
                        {
                            state = BulkCR;
                        }
                    }
                }
                else
                {
                    std::stack<State>().swap(states);
                    return std::make_pair(position, Error);
                }
                break;
            case Bulk: {
                assert( bulkSize > 0 );

                long int available = size - position + 1;
                long int canRead = std::min(available, bulkSize);

                buf.insert(buf.end(), ptr + position - 1, ptr + position - 1 + canRead);
                bulkSize -= canRead;
                position += canRead - 1;

                if( bulkSize == 0 )
                {
                    state = BulkCR;
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
                    std::stack<State>().swap(states);
                    return std::make_pair(position, Error);
                }
                break;
            case BulkLF:
                if( c == '\n')
                {
                    state = Start;
                    redisValue = RedisValue(buf);
                }
                else
                {
                    std::stack<State>().swap(states);
                    return std::make_pair(position, Error);
                }
                break;
            case ArraySize:
                if( c == '\r' )
                {
                    if( buf.empty() )
                    {
                        std::stack<State>().swap(states);
                        return std::make_pair(position, Error);
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
                    std::stack<State>().swap(states);
                    return std::make_pair(position, Error);
                }
                break;
            case ArraySizeLF:
                if( c == '\n' )
                {
                    int64_t arraySize = bufToLong(buf.data(), buf.size());
                    std::vector<RedisValue> array;

                    if( arraySize == -1 )
                    {
                        state = Start;
                        redisValue = RedisValue();  // Nil value
                    }
                    else if( arraySize == 0 )
                    {
                        state = Start;
                        redisValue = RedisValue(std::move(array));  // Empty array
                    }
                    else if( arraySize < 0 )
                    {
                        std::stack<State>().swap(states);
                        return std::make_pair(position, Error);
                    }
                    else
                    {
                        array.reserve(arraySize);
                        arraySizes.push(arraySize);
                        arrayValues.push(std::move(array));

                        state = StartArray;
                    }
                }
                else
                {
                    std::stack<State>().swap(states);
                    return std::make_pair(position, Error);
                }
                break;
            case Integer:
                if( c == '\r' )
                {
                    if( buf.empty() )
                    {
                        std::stack<State>().swap(states);
                        return std::make_pair(position, Error);
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
                    std::stack<State>().swap(states);
                    return std::make_pair(position, Error);
                }
                break;
            case IntegerLF:
                if( c == '\n' )
                {
                    int64_t value = bufToLong(buf.data(), buf.size());

                    buf.clear();
                    redisValue = RedisValue(value);
                    state = Start;
                }
                else
                {
                    std::stack<State>().swap(states);
                    return std::make_pair(position, Error);
                }
                break;
            default:
                std::stack<State>().swap(states);
                return std::make_pair(position, Error);
        }


        if (state == Start)
        {
            if (!arraySizes.empty())
            {
                assert(arraySizes.size() > 0);
                arrayValues.top().getArray().push_back(redisValue);

                while(!arraySizes.empty() && --arraySizes.top() == 0)
                {
                    arraySizes.pop();
                    redisValue = std::move(arrayValues.top());
                    arrayValues.pop();

                    if (!arraySizes.empty())
                        arrayValues.top().getArray().push_back(redisValue);
                }
            }


            if (arraySizes.empty())
            {
                // done
                break;
            }
        }
    }

    if (arraySizes.empty() && state == Start)
    {
        return std::make_pair(position, Completed);
    }
    else
    {
        states.push(state);
        return std::make_pair(position, Incompleted);
    }
}

RedisValue RedisParser::result()
{
    return std::move(redisValue);
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
    : pimpl(std::make_shared<RedisClientImpl>(ioService)),
    connectTimeout(boost::posix_time::hours(365 * 24)),
    commandTimeout(boost::posix_time::hours(365 * 24)),
    tcpNoDelay(true), tcpKeepAlive(false)
{
    pimpl->errorHandler = std::bind(&RedisClientImpl::defaulErrorHandler, std::placeholders::_1);
}

RedisSyncClient::RedisSyncClient(RedisSyncClient &&other)
    : pimpl(std::move(other.pimpl)),
    connectTimeout(std::move(other.connectTimeout)),
    commandTimeout(std::move(other.commandTimeout)),
    tcpNoDelay(std::move(other.tcpNoDelay)),
    tcpKeepAlive(std::move(other.tcpKeepAlive))
{
}


RedisSyncClient::~RedisSyncClient()
{
    if (pimpl)
        pimpl->close();
}

void RedisSyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint)
{
    boost::system::error_code ec;

    connect(endpoint, ec);
    detail::throwIfError(ec);
}

void RedisSyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint,
    boost::system::error_code &ec)
{
    pimpl->socket.open(endpoint.protocol(), ec);

    if (!ec && tcpNoDelay)
        pimpl->socket.set_option(boost::asio::ip::tcp::no_delay(true), ec);

    // TODO keep alive option

    // boost asio does not support `connect` with timeout
    int socket = pimpl->socket.native_handle();
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(endpoint.port());
    addr.sin_addr.s_addr = inet_addr(endpoint.address().to_string().c_str());

    // Set non-blocking
    int arg = 0;
    if ((arg = fcntl(socket, F_GETFL, NULL)) < 0)
    {
        ec = boost::system::error_code(errno,
                boost::asio::error::get_system_category());
        return;
    }

    arg |= O_NONBLOCK;

    if (fcntl(socket, F_SETFL, arg) < 0)
    {
        ec = boost::system::error_code(errno,
                boost::asio::error::get_system_category());
        return;
    }

    // connecting
    int result = ::connect(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (result < 0)
    {
        if (errno == EINPROGRESS)
        {
            for(;;)
            {
                //selecting
                pollfd pfd;
                pfd.fd = socket;
                pfd.events = POLLOUT;

                result = ::poll(&pfd, 1, connectTimeout.total_milliseconds());

                if (result < 0)
                {
                    if (errno == EINTR)
                    {
                        // try again
                        continue;
                    }
                    else
                    {
                        ec = boost::system::error_code(errno,
                                boost::asio::error::get_system_category());
                        return;
                    }
                }
                else if (result > 0)
                {
                    // check for error
                    int valopt;
                    socklen_t optlen = sizeof(valopt);


                    if (getsockopt(socket, SOL_SOCKET, SO_ERROR,
                                reinterpret_cast<void *>(&valopt), &optlen ) < 0)
                    {
                        ec = boost::system::error_code(errno,
                                boost::asio::error::get_system_category());
                        return;
                    }

                    if (valopt)
                    {
                        ec = boost::system::error_code(valopt,
                                boost::asio::error::get_system_category());
                        return;
                    }

                    break;
                }
                else
                {
                    // timeout
                    ec = boost::system::error_code(ETIMEDOUT,
                            boost::asio::error::get_system_category());
                    return;
                }
            }
        }
        else
        {
            ec = boost::system::error_code(errno,
                    boost::asio::error::get_system_category());
            return;
        }
    }

    if ((arg = fcntl(socket, F_GETFL, NULL)) < 0)
    {
        ec = boost::system::error_code(errno,
                boost::asio::error::get_system_category());
        return;
    }

    arg &= (~O_NONBLOCK); 

    if (fcntl(socket, F_SETFL, arg) < 0)
    {
        ec = boost::system::error_code(errno,
                boost::asio::error::get_system_category());
    }

    if (!ec)
        pimpl->state = State::Connected;
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

void RedisSyncClient::connect(const boost::asio::local::stream_protocol::endpoint &endpoint)
{
    boost::system::error_code ec;

    connect(endpoint, ec);
    detail::throwIfError(ec);
}

void RedisSyncClient::connect(const boost::asio::local::stream_protocol::endpoint &endpoint,
        boost::system::error_code &ec)
{
    pimpl->socket.open(endpoint.protocol(), ec);

    if (!ec)
        pimpl->socket.connect(endpoint, ec);

    if (!ec)
        pimpl->state = State::Connected;
}

#endif

bool RedisSyncClient::isConnected() const
{
    return pimpl->getState() == State::Connected ||
            pimpl->getState() == State::Subscribed;
}

void RedisSyncClient::disconnect()
{
    pimpl->close();
}

void RedisSyncClient::installErrorHandler(
        std::function<void(const std::string &)> handler)
{
    pimpl->errorHandler = std::move(handler);
}

RedisValue RedisSyncClient::command(std::string cmd, std::deque<RedisBuffer> args)
{
    boost::system::error_code ec;
    RedisValue result = command(std::move(cmd), std::move(args), ec);

    detail::throwIfError(ec);
    return result;
}

RedisValue RedisSyncClient::command(std::string cmd, std::deque<RedisBuffer> args,
            boost::system::error_code &ec)
{
    if(stateValid())
    {
        args.push_front(std::move(cmd));

        return pimpl->doSyncCommand(args, commandTimeout, ec);
    }
    else
    {
        return RedisValue();
    }
}

Pipeline RedisSyncClient::pipelined()
{
    Pipeline pipe(*this);
    return pipe;
}

RedisValue RedisSyncClient::pipelined(std::deque<std::deque<RedisBuffer>> commands)
{
    boost::system::error_code ec;
    RedisValue result = pipelined(std::move(commands), ec);

    detail::throwIfError(ec);
    return result;
}

RedisValue RedisSyncClient::pipelined(std::deque<std::deque<RedisBuffer>> commands,
        boost::system::error_code &ec)
{
    if(stateValid())
    {
        return pimpl->doSyncCommand(commands, commandTimeout, ec);
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

RedisSyncClient &RedisSyncClient::setConnectTimeout(
        const boost::posix_time::time_duration &timeout)
{
    connectTimeout = timeout;
    return *this;
}


RedisSyncClient &RedisSyncClient::setCommandTimeout(
        const boost::posix_time::time_duration &timeout)
{
    commandTimeout = timeout;
    return *this;
}

RedisSyncClient &RedisSyncClient::setTcpNoDelay(bool enable)
{
    tcpNoDelay = enable;
    return *this;
}

RedisSyncClient &RedisSyncClient::setTcpKeepAlive(bool enable)
{
    tcpKeepAlive = enable;
    return *this;
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

std::vector<char> &RedisValue::getByteArray()
{
    assert(isByteArray());
    return boost::get<std::vector<char>>(value);
}

const std::vector<char> &RedisValue::getByteArray() const
{
    assert(isByteArray());
    return boost::get<std::vector<char>>(value);
}

std::vector<RedisValue> &RedisValue::getArray()
{
    assert(isArray());
    return boost::get<std::vector<RedisValue>>(value);
}

const std::vector<RedisValue> &RedisValue::getArray() const
{
    assert(isArray());
    return boost::get<std::vector<RedisValue>>(value);
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

