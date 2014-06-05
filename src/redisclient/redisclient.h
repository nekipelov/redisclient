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
    /// Subscribe handle.
    struct Handle {
        size_t id;
        std::string channel;
    };

    REDIS_CLIENT_DECL RedisClient(boost::asio::io_service &ioService);
    REDIS_CLIENT_DECL ~RedisClient();

    REDIS_CLIENT_DECL bool connect(const char *address, int port);

    REDIS_CLIENT_DECL void asyncConnect(
            const boost::asio::ip::tcp::endpoint &endpoint,
            const boost::function<void(const boost::system::error_code &)> &handler);

    REDIS_CLIENT_DECL void installErrorHandler(const boost::function<void(const std::string &)> &handler);

    REDIS_CLIENT_DECL void command(
            const std::string &cmd,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1, const std::string &arg2,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1,
            const std::string &arg2, const std::string &arg3,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1, const std::string &arg2,
            const std::string &arg3, const std::string &arg4,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1,
            const std::string &arg2, const std::string &arg3,
            const std::string &arg4, const std::string &arg5,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1,
            const std::string &arg2, const std::string &arg3,
            const std::string &arg4, const std::string &arg5,
            const std::string &arg6,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);


    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::string &arg1,
            const std::string &arg2, const std::string &arg3,
            const std::string &arg4, const std::string &arg5,
            const std::string &arg6, const std::string &arg7,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL void command(
            const std::string &cmd, const std::list<std::string> &args,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL Handle subscribe(
            const std::string &channel,
            const boost::function<void(const std::string &msg)> &msgHandler,

            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL void unsubscribe(const Handle &handle);

    REDIS_CLIENT_DECL void singleShotSubscribe(
            const std::string &channel,
            const boost::function<void(const std::string &msg)> &msgHandler,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL void publish(
            const std::string &channel, const std::string &msg,
            const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    REDIS_CLIENT_DECL static void dummyHandler(const RedisValue &) {}

protected:
    REDIS_CLIENT_DECL void checkState() const;

private:
    RedisClientImpl impl;
};

#ifdef REDIS_CLIENT_HEADER_ONLY
#include "impl/redisclient.cpp"
#endif

#endif // REDISCLIENT_REDISCLIENT_H
