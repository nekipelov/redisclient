/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISSYNCCLIENT_CPP
#define REDISCLIENT_REDISSYNCCLIENT_CPP

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

#include "../redissyncclient.h"

RedisSyncClient::RedisSyncClient(boost::asio::io_service &ioService)
    : pimpl(boost::make_shared<RedisClientImpl>(boost::ref(ioService)))
{
    pimpl->errorHandler = boost::bind(&RedisClientImpl::defaulErrorHandler,
                                      pimpl, _1);
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
        pimpl->state = RedisClientImpl::Connected;
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

void RedisSyncClient::installErrorHandler(
        const boost::function<void(const std::string &)> &handler)
{
    pimpl->errorHandler = handler;
}

RedisValue RedisSyncClient::command(const std::string &s)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(1);
        items[0] = s;

        return pimpl->doSyncCommand(items);
    }
    else
    {
        return RedisValue();
    }
}

RedisValue RedisSyncClient::command(const std::string &cmd, const RedisBuffer &arg1)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(2);
        items[0] = cmd;
        items[1] = arg1;

        return pimpl->doSyncCommand(items);
    }
    else
    {
        return RedisValue();
    }
}

RedisValue RedisSyncClient::command(const std::string &cmd, const RedisBuffer &arg1,
        const RedisBuffer &arg2)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(3);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2; 

        return pimpl->doSyncCommand(items);
    }
    else
    {
        return RedisValue();
    }
}

RedisValue RedisSyncClient::command(const std::string &cmd, const RedisBuffer &arg1,
                          const RedisBuffer &arg2, const RedisBuffer &arg3)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(4);
        items[0] = cmd;
        items[1] = arg1; 
        items[2] = arg2;
        items[3] = arg3;

        return pimpl->doSyncCommand(items);
    }
    else
    {
        return RedisValue();
    }
}

RedisValue RedisSyncClient::command(const std::string &cmd, const RedisBuffer &arg1,
                          const RedisBuffer &arg2, const RedisBuffer &arg3,
                          const RedisBuffer &arg4)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(5);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;
        items[3] = arg3;
        items[4] = arg4;

        return pimpl->doSyncCommand(items);
    }
    else
    {
        return RedisValue();
    }
}

RedisValue RedisSyncClient::command(const std::string &cmd, const RedisBuffer &arg1,
                          const RedisBuffer &arg2, const RedisBuffer &arg3,
                          const RedisBuffer &arg4, const RedisBuffer &arg5)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(6);
        items[0] = cmd;
        items[1] = arg1; 
        items[2] = arg2;
        items[3] = arg3;
        items[4] = arg4;
        items[5] = arg5;

        return pimpl->doSyncCommand(items);
    }
    else
    {
        return RedisValue();
    }
}

RedisValue RedisSyncClient::command(const std::string &cmd, const RedisBuffer &arg1,
                          const RedisBuffer &arg2, const RedisBuffer &arg3,
                          const RedisBuffer &arg4, const RedisBuffer &arg5,
                          const RedisBuffer &arg6)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(7);
        items[0] = cmd;
        items[1] = arg1; 
        items[2] = arg2;
        items[3] = arg3;
        items[4] = arg4;
        items[5] = arg5;
        items[6] = arg6;

        return pimpl->doSyncCommand(items);
    }
    else
    {
        return RedisValue();
    }
}

RedisValue RedisSyncClient::command(const std::string &cmd, const RedisBuffer &arg1,
                          const RedisBuffer &arg2, const RedisBuffer &arg3,
                          const RedisBuffer &arg4, const RedisBuffer &arg5,
                          const RedisBuffer &arg6, const RedisBuffer &arg7)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(8);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;
        items[3] = arg3;
        items[4] = arg4;
        items[5] = arg5;
        items[6] = arg6;
        items[7] = arg7;

        return pimpl->doSyncCommand(items);
    }
    else
    {
        return RedisValue();
    }
}

RedisValue RedisSyncClient::command(const std::string &cmd, const std::list<std::string> &args)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(1);
        items[0] = cmd;

        items.reserve(1 + args.size());

        std::copy(args.begin(), args.end(), std::back_inserter(items));
        return pimpl->doSyncCommand(items);
    }
    else
    {
        return RedisValue();
    }
}

bool RedisSyncClient::stateValid() const
{
    assert( pimpl->state == RedisClientImpl::Connected );

    if( pimpl->state != RedisClientImpl::Connected )
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << pimpl->state;

        pimpl->errorHandler(ss.str());
        return false;
    }

    return true;
}

#endif // REDISCLIENT_REDISSYNCCLIENT_CPP
