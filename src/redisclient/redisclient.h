/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISCLIENT_H
#define REDISCLIENT_REDISCLIENT_H

#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <list>

#include "impl/redisclientimpl.h"
#include "redisvalue.h"
#include "config.h"

class RedisClientImpl;

class RedisClient : boost::noncopyable {
public:
    // Subscribe handle.
    struct Handle {
        size_t id;
        std::string channel;
    };

    REDIS_CLIENT_DECL RedisClient(boost::asio::io_service &ioService);
    REDIS_CLIENT_DECL ~RedisClient();

    // Connect to redis server, blocking call.
    REDIS_CLIENT_DECL bool connect(const boost::asio::ip::address &address,
                                   unsigned short port);

    // Connect to redis server, asynchronous call.
    REDIS_CLIENT_DECL void asyncConnect(
            const boost::asio::ip::address &address,
            unsigned short port,
            const boost::function<void(bool, const std::string &)> &handler);

    // Connect to redis server, asynchronous call.
    REDIS_CLIENT_DECL void asyncConnect(
            const boost::asio::ip::tcp::endpoint &endpoint,
            const boost::function<void(bool, const std::string &)> &handler);

    // Set custom error handler. 
    REDIS_CLIENT_DECL void installErrorHandler(
        const boost::function<void(const std::string &)> &handler);

    // Execute command on Redis server.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with one argument.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with two arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1, const std::string &arg2,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with three arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1,
            const std::string &arg2, const std::string &arg3,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with four arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1, const std::string &arg2,
            const std::string &arg3, const std::string &arg4,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with five arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1,
            const std::string &arg2, const std::string &arg3,
            const std::string &arg4, const std::string &arg5,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with six arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1,
            const std::string &arg2, const std::string &arg3,
            const std::string &arg4, const std::string &arg5,
            const std::string &arg6,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);


    // Execute command on Redis server with seven arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1,
            const std::string &arg2, const std::string &arg3,
            const std::string &arg4, const std::string &arg5,
            const std::string &arg6, const std::string &arg7,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Execute command on Redis server with the list of arguments.
    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::list<std::string> &args,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Subscribe to channel. Handler msgHandler will be called
    // when someone publish message on channel. Call unsubscribe 
    // to stop the subscription.
    REDIS_CLIENT_DECL Handle subscribe(
            const std::string &channelName,
            const boost::function<void(const std::string &msg)> &msgHandler,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Unsubscribe
    REDIS_CLIENT_DECL void unsubscribe(const Handle &handle);

    // Subscribe to channel. Handler msgHandler will be called
    // when someone publish message on channel; it will be 
    // unsubscribed after call.
    REDIS_CLIENT_DECL void singleShotSubscribe(
            const std::string &channel,
            const boost::function<void(const std::string &msg)> &msgHandler,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    // Publish message on channel.
    REDIS_CLIENT_DECL void publish(
            const std::string &channel, const std::string &msg,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL static void dummyHandler(const RedisValue &) {}

protected:
    REDIS_CLIENT_DECL bool stateValid() const;

private:
    boost::shared_ptr<RedisClientImpl> pimpl;
};

#ifdef REDIS_CLIENT_HEADER_ONLY
#include "impl/redisclient.cpp"
#endif

#endif // REDISCLIENT_REDISCLIENT_H
