/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISCLIENTIMPL_H
#define REDISCLIENT_REDISCLIENTIMPL_H

#include <boost/array.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <string>
#include <vector>
#include <queue>
#include <map>

#include "../redisclient.h"
#include "../redisparser.h"
#include "../config.h"

class RedisClientImpl : public boost::enable_shared_from_this<RedisClientImpl> {
public:
    REDIS_CLIENT_DECL RedisClientImpl(boost::asio::io_service &ioService);
    REDIS_CLIENT_DECL ~RedisClientImpl();

    REDIS_CLIENT_DECL void handleAsyncConnect(
            const boost::system::error_code &ec,
            const boost::function<void(bool, const std::string &)> &handler);

    REDIS_CLIENT_DECL void close();

    REDIS_CLIENT_DECL void doCommand(
            const std::vector<std::string> &command,
            const boost::function<void(const RedisValue &)> &handler);

    REDIS_CLIENT_DECL void sendNextCommand();
    REDIS_CLIENT_DECL void processMessage();
    REDIS_CLIENT_DECL void doProcessMessage(const RedisValue &v);
    REDIS_CLIENT_DECL void asyncWrite(const boost::system::error_code &ec, const size_t);
    REDIS_CLIENT_DECL void asyncRead(const boost::system::error_code &ec, const size_t);

    REDIS_CLIENT_DECL void onRedisError(const RedisValue &);
    REDIS_CLIENT_DECL void onError(const std::string &s);
    REDIS_CLIENT_DECL void defaulErrorHandler(const std::string &s);

    REDIS_CLIENT_DECL static void append(std::vector<char> &vec, const std::string &s);
    REDIS_CLIENT_DECL static void append(std::vector<char> &vec, const char *s);
    REDIS_CLIENT_DECL static void append(std::vector<char> &vec, char c);
    template<size_t size>
    REDIS_CLIENT_DECL static void append(std::vector<char> &vec, const char s[size]);

    template<typename Handler>
    REDIS_CLIENT_DECL void post(const Handler &handler);

    enum {
        NotConnected,
        Connected,
        Subscribed,
        Closed 
    } state;

    boost::asio::strand strand;
    boost::asio::ip::tcp::socket socket;
    RedisParser redisParser;
    boost::array<char, 4096> buf;
    size_t subscribeSeq;

    typedef std::pair<size_t, boost::function<void(const std::string &s)> > MsgHandlerType;
    typedef boost::function<void(const std::string &s)> SingleShotHandlerType;

    typedef std::multimap<std::string, MsgHandlerType> MsgHandlersMap;
    typedef std::multimap<std::string, SingleShotHandlerType> SingleShotHandlersMap;

    std::queue<boost::function<void(const RedisValue &v)> > handlers;
    MsgHandlersMap msgHandlers;
    SingleShotHandlersMap singleShotMsgHandlers;

    struct QueueItem {
        boost::function<void(const RedisValue &)> handler;
        boost::shared_ptr<std::vector<char> > buff;
    };

    std::queue<QueueItem> queue;

    boost::function<void(const std::string &)> errorHandler;
};

#ifdef REDIS_CLIENT_HEADER_ONLY
#include "redisclientimpl.cpp"
#endif

#endif // REDISCLIENT_REDISCLIENTIMPL_H
