/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISCLIENTIMPL_CPP
#define REDISCLIENT_REDISCLIENTIMPL_CPP

#include <boost/asio/write.hpp>

#include <algorithm>

#include "redisclientimpl.h"

namespace
{
    static const char crlf[] = {'\r', '\n'};
    inline void bufferAppend(std::vector<char> &vec, const std::string &s);
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
    if( state != State::Closed )
    {
        boost::system::error_code ignored_ec;

        msgHandlers.clear();
        decltype(handlers)().swap(handlers);

        socket.cancel(ignored_ec);
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
        errorHandler(ec.message());
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
        errorHandler(ec.message());
        return RedisValue();
    }

    std::vector<RedisValue> responses;

    for(size_t i = 0; i < commands.size(); ++i)
    {
        responses.push_back(syncReadResponse(timeout, ec));

        if (ec)
        {
            errorHandler(ec.message());
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
