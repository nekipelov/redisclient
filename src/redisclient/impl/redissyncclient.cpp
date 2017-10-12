/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISSYNCCLIENT_CPP
#define REDISCLIENT_REDISSYNCCLIENT_CPP

#include <memory>
#include <functional>

#include "redisclient/redissyncclient.h"
#include "redisclient/pipeline.h"

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

RedisValue RedisSyncClient::command(std::string cmd, std::deque<RedisBuffer> args)
{
    if(stateValid())
    {
        args.push_front(std::move(cmd));

        return pimpl->doSyncCommand(args);
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
    if(stateValid())
    {
        return pimpl->doSyncCommand(commands);
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
