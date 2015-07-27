/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISCLIENTIMPL_CPP
#define REDISCLIENT_REDISCLIENTIMPL_CPP

#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>

#include "redisclientimpl.h"

RedisClientImpl::RedisClientImpl(boost::asio::io_service &ioService)
    : state(NotConnected), strand(ioService), socket(ioService), subscribeSeq(0)
{
}

RedisClientImpl::~RedisClientImpl()
{
    close();
}

void RedisClientImpl::close()
{
    if( state != RedisClientImpl::Closed )
    {
        boost::system::error_code ignored_ec;

        errorHandler = boost::bind(&RedisClientImpl::ignoreErrorHandler, _1);
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        state = RedisClientImpl::Closed;
    }
}


void RedisClientImpl::processMessage()
{
    using boost::system::error_code;

    socket.async_read_some(boost::asio::buffer(buf),
                           boost::bind(&RedisClientImpl::asyncRead,
                                       shared_from_this(), _1, _2));
}

void RedisClientImpl::doProcessMessage(const RedisValue &v)
{
    if( state == RedisClientImpl::Subscribed )
    {
        std::vector<RedisValue> result = v.toArray();

        if( result.size() == 3 )
        {
            const RedisValue &command = result[0];
            const RedisValue &queueName = result[1];
            const RedisValue &value = result[2];

            const std::string &cmd = command.toString();

            if( cmd == "message" )
            {
                SingleShotHandlersMap::iterator it = singleShotMsgHandlers.find(queueName.toString());
                if( it != singleShotMsgHandlers.end() )
                {
                    strand.post(boost::bind(it->second, value.toByteArray()));
                    singleShotMsgHandlers.erase(it);
                }

                std::pair<MsgHandlersMap::iterator, MsgHandlersMap::iterator> pair =
                        msgHandlers.equal_range(queueName.toString());
                for(MsgHandlersMap::iterator handlerIt = pair.first;
                    handlerIt != pair.second; ++handlerIt)
                {
                    strand.post(boost::bind(handlerIt->second.second, value.toByteArray()));
                }
            }
            else if( cmd == "subscribe" && handlers.empty() == false )
            {
                handlers.front()(v);
                handlers.pop();
            }
            else if(cmd == "unsubscribe" && handlers.empty() == false )
            {
                handlers.front()(v);
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
            handlers.front()(v);
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

void RedisClientImpl::asyncWrite(const boost::system::error_code &ec, const size_t)
{
    if( ec )
    {
        errorHandler(ec.message());
        return;
    }

    assert(queue.empty() == false);
    queue.pop();

    if( queue.empty() == false )
    {
        const QueueItem &item = queue.front();
        
        boost::asio::async_write(socket,
                                 boost::asio::buffer(item.buff->data(), item.buff->size()),
                                 boost::bind(&RedisClientImpl::asyncWrite, shared_from_this(), _1, _2));
    }
}

void RedisClientImpl::handleAsyncConnect(const boost::system::error_code &ec,
                                         const boost::function<void(bool, const std::string &)> &handler)
{
    if( !ec )
    {
        socket.set_option(boost::asio::ip::tcp::no_delay(true));
        state = RedisClientImpl::Connected;
        handler(true, std::string());
        processMessage();
    }
    else
    {
        handler(false, ec.message());
    }
}

std::vector<char> RedisClientImpl::makeCommand(const std::vector<RedisBuffer> &items)
{
    static const char crlf[] = {'\r', '\n'};

    std::vector<char> result;

    append(result, '*');
    append(result, boost::lexical_cast<std::string>(items.size()));
    append<>(result, crlf);

    std::vector<RedisBuffer>::const_iterator it = items.begin(), end = items.end();
    for(; it != end; ++it)
    {
        append(result, '$');
        append(result, boost::lexical_cast<std::string>(it->size()));
        append<>(result, crlf);
        append(result, *it);
        append<>(result, crlf);
    }

    return result;
}

RedisValue RedisClientImpl::doSyncCommand(const std::vector<RedisBuffer> &buff)
{
    assert( queue.empty() );

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

void RedisClientImpl::doAsyncCommand(const std::vector<char> &buff,
                                     const boost::function<void(const RedisValue &)> &handler)
{
    QueueItem item;

    item.buff.reset( new std::vector<char>(buff) );
    item.handler = handler;
    queue.push(item);

    handlers.push( item.handler );

    if( queue.size() == 1 )
    {
        boost::asio::async_write(socket, 
                                 boost::asio::buffer(item.buff->data(), item.buff->size()),
                                 boost::bind(&RedisClientImpl::asyncWrite, shared_from_this(), _1, _2));
    }
}

void RedisClientImpl::asyncRead(const boost::system::error_code &ec, const size_t size)
{
    if( ec || size == 0 )
    {
        errorHandler(ec.message());
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
    std::string message = v.toString();
    errorHandler(message);
}


void RedisClientImpl::defaulErrorHandler(const std::string &s)
{
    throw std::runtime_error(s);
}

void RedisClientImpl::ignoreErrorHandler(const std::string &)
{
    // empty
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

#endif // REDISCLIENT_REDISCLIENTIMPL_CPP
