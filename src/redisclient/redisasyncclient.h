/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISASYNCCLIENT_REDISCLIENT_H
#define REDISASYNCCLIENT_REDISCLIENT_H

#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <list>

#include "impl/redisclientimpl.h"
#include "redisvalue.h"
#include "redisbuffer.h"
#include "config.h"

class RedisClientImpl;

class RedisAsyncClient : boost::noncopyable {
public:
    // Subscribe handle.
    struct Handle {
        size_t id;
        std::string channel;
    };

    REDIS_CLIENT_DECL RedisAsyncClient(boost::asio::io_service &ioService);
    REDIS_CLIENT_DECL ~RedisAsyncClient();

    // Connect to redis server
    REDIS_CLIENT_DECL void connect(
            const boost::asio::ip::address &address,
            unsigned short port,
            const boost::function<void(bool, const std::string &)> &handler);

    // Connect to redis server
    REDIS_CLIENT_DECL void connect(
            const boost::asio::ip::tcp::endpoint &endpoint,
            const boost::function<void(bool, const std::string &)> &handler);

    // backward compatibility
    inline void asyncConnect(
            const boost::asio::ip::address &address,
            unsigned short port,
            const boost::function<void(bool, const std::string &)> &handler)
    {
        connect(address, port, handler);
    }

    // backward compatibility
    inline void asyncConnect(
            const boost::asio::ip::tcp::endpoint &endpoint,
            const boost::function<void(bool, const std::string &)> &handler)
    {
        connect(endpoint, handler);
    }

    // return true if is connected to redis
    REDIS_CLIENT_DECL bool isConnected() const;

    // disconnect from redis and clear command queue
    REDIS_CLIENT_DECL void disconnect();

    // Set custom error handler. 
    REDIS_CLIENT_DECL void installErrorHandler(
        const boost::function<void(const std::string &)> &handler);

    // Execute command on Redis server.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with one argument.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const RedisBuffer &arg1,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with two arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const RedisBuffer &arg1, const RedisBuffer &arg2,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with three arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const RedisBuffer &arg1,
            const RedisBuffer &arg2, const RedisBuffer &arg3,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with four arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const RedisBuffer &arg1, const RedisBuffer &arg2,
            const RedisBuffer &arg3, const RedisBuffer &arg4,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with five arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const RedisBuffer &arg1,
            const RedisBuffer &arg2, const RedisBuffer &arg3,
            const RedisBuffer &arg4, const RedisBuffer &arg5,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with six arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const RedisBuffer &arg1,
            const RedisBuffer &arg2, const RedisBuffer &arg3,
            const RedisBuffer &arg4, const RedisBuffer &arg5,
            const RedisBuffer &arg6,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);


    // Execute command on Redis server with seven arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const RedisBuffer &arg1,
            const RedisBuffer &arg2, const RedisBuffer &arg3,
            const RedisBuffer &arg4, const RedisBuffer &arg5,
            const RedisBuffer &arg6, const RedisBuffer &arg7,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with the list of arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::list<RedisBuffer> &args,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Subscribe to channel. Handler msgHandler will be called
    // when someone publish message on channel. Call unsubscribe 
    // to stop the subscription.
    REDIS_CLIENT_DECL Handle subscribe(
            const std::string &channelName,
            const boost::function<void(const std::vector<char> &msg)> &msgHandler,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Unsubscribe
    REDIS_CLIENT_DECL void unsubscribe(const Handle &handle);

    // Subscribe to channel. Handler msgHandler will be called
    // when someone publish message on channel; it will be 
    // unsubscribed after call.
    REDIS_CLIENT_DECL void singleShotSubscribe(
            const std::string &channel,
            const boost::function<void(const std::vector<char> &msg)> &msgHandler,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Publish message on channel.
    REDIS_CLIENT_DECL void publish(
            const std::string &channel, const RedisBuffer &msg,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL static void dummyHandler(const RedisValue &) {}

protected:
    REDIS_CLIENT_DECL bool stateValid() const;

private:
    boost::shared_ptr<RedisClientImpl> pimpl;
};

#ifdef REDIS_CLIENT_HEADER_ONLY
#include "impl/redisasyncclient.cpp"
#endif

#endif // REDISASYNCCLIENT_REDISCLIENT_H
