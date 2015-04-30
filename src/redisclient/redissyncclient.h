/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISSYNCCLIENT_REDISCLIENT_H
#define REDISSYNCCLIENT_REDISCLIENT_H

#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <list>

#include "impl/redisclientimpl.h"
#include "redisbuffer.h"
#include "redisvalue.h"
#include "config.h"

class RedisClientImpl;

class RedisSyncClient : boost::noncopyable {
public:
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

    // Set custom error handler. 
    REDIS_CLIENT_DECL void installErrorHandler(
        const boost::function<void(const std::string &)> &handler);

    // Execute command on Redis server.
    REDIS_CLIENT_DECL RedisValue command(const std::string &cmd);

    // Execute command on Redis server with one argument.
    REDIS_CLIENT_DECL RedisValue command(const std::string &cmd, const RedisBuffer &arg1);

    // Execute command on Redis server with two arguments.
    REDIS_CLIENT_DECL RedisValue command(
            const std::string &cmd, const RedisBuffer &arg1, const RedisBuffer &arg2);

    // Execute command on Redis server with three arguments.
    REDIS_CLIENT_DECL RedisValue command(
            const std::string &cmd, const RedisBuffer &arg1,
            const RedisBuffer &arg2, const RedisBuffer &arg3);

    // Execute command on Redis server with four arguments.
    REDIS_CLIENT_DECL RedisValue command(
            const std::string &cmd, const RedisBuffer &arg1, const RedisBuffer &arg2,
            const RedisBuffer &arg3, const RedisBuffer &arg4);

    // Execute command on Redis server with five arguments.
    REDIS_CLIENT_DECL RedisValue command(
            const std::string &cmd, const RedisBuffer &arg1,
            const RedisBuffer &arg2, const RedisBuffer &arg3,
            const RedisBuffer &arg4, const RedisBuffer &arg5);

    // Execute command on Redis server with six arguments.
    REDIS_CLIENT_DECL RedisValue command(
            const std::string &cmd, const RedisBuffer &arg1,
            const RedisBuffer &arg2, const RedisBuffer &arg3,
            const RedisBuffer &arg4, const RedisBuffer &arg5,
            const RedisBuffer &arg6);

    // Execute command on Redis server with seven arguments.
    REDIS_CLIENT_DECL RedisValue command(
            const std::string &cmd, const RedisBuffer &arg1,
            const RedisBuffer &arg2, const RedisBuffer &arg3,
            const RedisBuffer &arg4, const RedisBuffer &arg5,
            const RedisBuffer &arg6, const RedisBuffer &arg7);

    // Execute command on Redis server with the list of arguments.
    REDIS_CLIENT_DECL RedisValue command(
            const std::string &cmd, const std::list<std::string> &args);

protected:
    REDIS_CLIENT_DECL bool stateValid() const;

private:
    boost::shared_ptr<RedisClientImpl> pimpl;
};

#ifdef REDIS_CLIENT_HEADER_ONLY
#include "impl/redissyncclient.cpp"
#endif

#endif // REDISSYNCCLIENT_REDISCLIENT_H
