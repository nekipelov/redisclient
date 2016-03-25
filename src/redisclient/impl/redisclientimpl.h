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

#include "../redisparser.h"
#include "../redisbuffer.h"
#include "../config.h"

class RedisClientImpl : public boost::enable_shared_from_this<RedisClientImpl> {
public:
    enum State {
        NotConnected,
        Connected,
        Subscribed,
        Closed
    };

    REDIS_CLIENT_DECL RedisClientImpl(boost::asio::io_service &ioService);
    REDIS_CLIENT_DECL ~RedisClientImpl();

    REDIS_CLIENT_DECL void handleAsyncConnect(
            const boost::system::error_code &ec,
            const boost::function<void(bool, const std::string &)> &handler);

    REDIS_CLIENT_DECL void close();

    REDIS_CLIENT_DECL State getState() const;

    REDIS_CLIENT_DECL static std::vector<char> makeCommand(const std::vector<RedisBuffer> &items);

    REDIS_CLIENT_DECL RedisValue doSyncCommand(const std::vector<RedisBuffer> &buff);

    REDIS_CLIENT_DECL void doAsyncCommand(
            const std::vector<char> &buff,
            const boost::function<void(const RedisValue &)> &handler);

    REDIS_CLIENT_DECL void sendNextCommand();
    REDIS_CLIENT_DECL void processMessage();
    REDIS_CLIENT_DECL void doProcessMessage(const RedisValue &v);
    REDIS_CLIENT_DECL void asyncWrite(const boost::system::error_code &ec, const size_t);
    REDIS_CLIENT_DECL void asyncRead(const boost::system::error_code &ec, const size_t);

    REDIS_CLIENT_DECL void onRedisError(const RedisValue &);
    REDIS_CLIENT_DECL void defaulErrorHandler(const std::string &s);

    REDIS_CLIENT_DECL static void append(std::vector<char> &vec, const RedisBuffer &buf);
    REDIS_CLIENT_DECL static void append(std::vector<char> &vec, const std::string &s);
    REDIS_CLIENT_DECL static void append(std::vector<char> &vec, const char *s);
    REDIS_CLIENT_DECL static void append(std::vector<char> &vec, char c);
    template<size_t size>
    static inline void append(std::vector<char> &vec, const char (&s)[size]);

    template<typename Handler>
    inline void post(const Handler &handler);

    boost::asio::strand strand;
    boost::asio::ip::tcp::socket socket;
    RedisParser redisParser;
    boost::array<char, 4096> buf;
    size_t subscribeSeq;

    typedef std::pair<size_t, boost::function<void(const std::vector<char> &buf)> > MsgHandlerType;
    typedef boost::function<void(const std::vector<char> &buf)> SingleShotHandlerType;

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
    State state;
};

template<size_t size>
void RedisClientImpl::append(std::vector<char> &vec, const char (&s)[size])
{
    vec.insert(vec.end(), s, s + size);
}

template<typename Handler>
inline void RedisClientImpl::post(const Handler &handler)
{
    strand.post(handler);
}


#ifdef REDIS_CLIENT_HEADER_ONLY
#include "redisclientimpl.cpp"
#endif

#endif // REDISCLIENT_REDISCLIENTIMPL_H
