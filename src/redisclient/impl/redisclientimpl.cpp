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

        errorHandler = boost::function<void(const std::string &)>(); 
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        socket.close(ignored_ec);
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
            const RedisValue &queue = result[1];
            const RedisValue &value = result[2];

            const std::string &cmd = command.toString();

            if( cmd == "message" )
            {
                SingleShotHandlersMap::iterator it = singleShotMsgHandlers.find(queue.toString());
                if( it != singleShotMsgHandlers.end() )
                {
                    strand.post(boost::bind(it->second, value.toString()));
                    singleShotMsgHandlers.erase(it);
                }

                std::pair<MsgHandlersMap::iterator, MsgHandlersMap::iterator> pair =
                        msgHandlers.equal_range(queue.toString());
                for(MsgHandlersMap::iterator it = pair.first; it != pair.second; ++it)
                    strand.post(boost::bind(it->second.second, value.toString()));
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

                onError(ss.str());
                return;
            }
        }
        else
        {
            onError("[RedisClient] Protocol error");
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

            onError(ss.str());
            return;
        }
    }
}

void RedisClientImpl::asyncWrite(const boost::system::error_code &ec, const size_t)
{
    if( ec )
    {
        onError(ec.message());
        return;
    }

    assert( queue.empty() == false );

    const QueueItem &item = queue.front();

    handlers.push( item.handler );
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

void RedisClientImpl::doCommand(const std::vector<std::string> &command,
                                const boost::function<void(const RedisValue &)> &handler)
{
    
    using boost::system::error_code;
    static const char crlf[] = {'\r', '\n'};

    QueueItem item;

    item.buff.reset( new std::vector<char>() );
    item.handler = handler;
    queue.push(item);

    append(*item.buff.get(), '*');
    append(*item.buff.get(), boost::lexical_cast<std::string>(command.size()));
    append(*item.buff.get(), crlf);

    std::vector<std::string>::const_iterator it = command.begin(), end = command.end();
    for(; it != end; ++it)
    {
        append(*item.buff.get(), '$');
        append(*item.buff.get(), boost::lexical_cast<std::string>(it->size()));
        append(*item.buff.get(), crlf);
        append(*item.buff.get(), boost::lexical_cast<std::string>(*it));
        append(*item.buff.get(), crlf);
    }

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
        onError(ec.message());
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
            onError("[RedisClient] Parser error");
            return;
        }

        pos += result.first;
    }

    processMessage();

}

void RedisClientImpl::onRedisError(const RedisValue &v)
{
    if( errorHandler )
    {
        std::string message = v.toString();
        errorHandler(message);
    }
}

void RedisClientImpl::onError(const std::string &s)
{
    if( errorHandler )
    {
        errorHandler(s);
    }
}

void RedisClientImpl::defaulErrorHandler(const std::string &s)
{
    throw std::runtime_error(s);
}

void RedisClientImpl::append(std::vector<char> &vec, const std::string &s)
{
    vec.insert(vec.end(), s.begin(), s.end());
}

void RedisClientImpl::append(std::vector<char> &vec, const char *s)
{
    vec.insert(vec.end(), s, s + strlen(s));
}

template<size_t size>
void RedisClientImpl::append(std::vector<char> &vec, const char s[size])
{
    vec.insert(vec.end(), s, s + size);
}

void RedisClientImpl::append(std::vector<char> &vec, char c)
{
    vec.resize(vec.size() + 1);
    vec[vec.size() - 1] = c;
}

template<typename Handler>
void RedisClientImpl::post(const Handler &handler)
{
    strand.post(handler);
}

#endif // REDISCLIENT_REDISCLIENTIMPL_CPP
