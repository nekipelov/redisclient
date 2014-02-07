/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <string>
#include <list>
#include <memory>

#include "redisvalue.h"

class RedisClientImpl;

class RedisClient {
public:
    /// Subscribe handle.
    struct Handle {
        size_t id;
        std::string channel;
    };

    RedisClient(boost::asio::io_service &ioService);
    ~RedisClient();

    bool connect(const char *address, int port);

    void asyncConnect(const boost::asio::ip::tcp::endpoint &endpoint,
                      const boost::function<void(const boost::system::error_code &)> &handler);

    void installErrorHandler(const boost::function<void(const std::string &)> &handler);

    void command(const std::string &cmd,
                 const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    void command(const std::string &cmd, const std::string &arg1,
                 const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    void command(const std::string &cmd, const std::string &arg1,
                 const std::string &arg2,
                 const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    void command(const std::string &cmd, const std::string &arg1,
                 const std::string &arg2, const std::string &arg3,
                 const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    void command(const std::string &cmd, const std::string &arg1,
                 const std::string &arg2, const std::string &arg3,
                 const std::string &arg4,
                 const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    void command(const std::string &cmd, const std::string &arg1,
                 const std::string &arg2, const std::string &arg3,
                 const std::string &arg4, const std::string &arg5,
                 const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    void command(const std::string &cmd, const std::string &arg1,
                 const std::string &arg2, const std::string &arg3,
                 const std::string &arg4, const std::string &arg5,
                 const std::string &arg6,
                 const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    void command(const std::string &cmd, const std::string &arg1,
                 const std::string &arg2, const std::string &arg3,
                 const std::string &arg4, const std::string &arg5,
                 const std::string &arg6, const std::string &arg7,
                 const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    void command(const std::string &cmd, const std::list<std::string> &args,
                 const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    Handle subscribe(const std::string &channel,
                     const boost::function<void(const std::string &msg)> &msgHandler,
                     const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    void unsubscribe(const Handle &handle);

    void singleShotSubscribe(const std::string &channel,
                             const boost::function<void(const std::string &msg)> &msgHandler,
                             const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    void publish(const std::string &channel, const std::string &msg,
                 const boost::function<void(const RedisValue &)> &handler = &dummyHandler);

    static void dummyHandler(const RedisValue &) {}

protected:
    void checkState() const;

private:
    boost::shared_ptr<RedisClientImpl> pimpl;
};

#endif // REDIS_CLIENT_H
