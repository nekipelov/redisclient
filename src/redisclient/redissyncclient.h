/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISSYNCCLIENT_REDISCLIENT_H
#define REDISSYNCCLIENT_REDISCLIENT_H

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <list>
#include <functional>

#include "impl/redisclientimpl.h"
#include "redisbuffer.h"
#include "redisvalue.h"
#include "config.h"

namespace redisclient {

class RedisClientImpl;

class RedisSyncClient : boost::noncopyable {
public:
    typedef RedisClientImpl::State State;

    REDIS_CLIENT_DECL RedisSyncClient(boost::asio::io_service &ioService);
    REDIS_CLIENT_DECL ~RedisSyncClient();

    // Connect to redis server
    REDIS_CLIENT_DECL bool connect(
            const boost::asio::ip::tcp::endpoint &endpoint,
            std::string &errmsg);

    // Connect to redis server
    REDIS_CLIENT_DECL bool connect(
            const boost::asio::ip::address &address,
            unsigned short port,
            std::string &errmsg);

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
    REDIS_CLIENT_DECL bool connect(
            const boost::asio::local::stream_protocol::endpoint &endpoint,
            std::string &errmsg);
#endif

    // Set custom error handler.
    REDIS_CLIENT_DECL void installErrorHandler(
        std::function<void(const std::string &)> handler);

    // Execute command on Redis server with the list of arguments.
    REDIS_CLIENT_DECL RedisValue command(
            const std::string &cmd, std::deque<RedisBuffer> args);

    // Return connection state. See RedisClientImpl::State.
    REDIS_CLIENT_DECL State state() const;

protected:
    REDIS_CLIENT_DECL bool stateValid() const;

private:
    std::shared_ptr<RedisClientImpl> pimpl;
};

}

#ifdef REDIS_CLIENT_HEADER_ONLY
#include "impl/redissyncclient.cpp"
#endif

#endif // REDISSYNCCLIENT_REDISCLIENT_H
