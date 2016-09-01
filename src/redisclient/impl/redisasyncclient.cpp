/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISASYNCCLIENT_REDISASYNCCLIENT_CPP
#define REDISASYNCCLIENT_REDISASYNCCLIENT_CPP

#include <memory>
#include <functional>

#include "../redisasyncclient.h"

namespace redisclient {

RedisAsyncClient::RedisAsyncClient(boost::asio::io_service &ioService)
    : pimpl(std::make_shared<RedisClientImpl>(ioService))
{
    pimpl->errorHandler = std::bind(&RedisClientImpl::defaulErrorHandler, std::placeholders::_1);
}

RedisAsyncClient::~RedisAsyncClient()
{
    pimpl->close();
}

void RedisAsyncClient::connect(const boost::asio::ip::address &address,
                               unsigned short port,
                               std::function<void(bool, const std::string &)> handler)
{
    boost::asio::ip::tcp::endpoint endpoint(address, port);
    connect(endpoint, std::move(handler));
}

void RedisAsyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint,
                               std::function<void(bool, const std::string &)> handler)
{
    pimpl->state = State::Connecting;
    pimpl->socket.async_connect(endpoint, std::bind(&RedisClientImpl::handleAsyncConnect,
                pimpl, std::placeholders::_1, std::move(handler)));
}

bool RedisAsyncClient::isConnected() const
{
    return pimpl->getState() == State::Connected ||
            pimpl->getState() == State::Subscribed;
}

void RedisAsyncClient::disconnect()
{
    pimpl->close();
}

void RedisAsyncClient::installErrorHandler(std::function<void(const std::string &)> handler)
{
    pimpl->errorHandler = std::move(handler);
}

void RedisAsyncClient::command(const std::string &cmd, std::deque<RedisBuffer> args,
                          std::function<void(RedisValue)> handler)
{
    if(stateValid())
    {
        args.emplace_front(cmd);

        pimpl->post(std::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    std::move(pimpl->makeCommand(args)), std::move(handler)));
    }
}

RedisAsyncClient::Handle RedisAsyncClient::subscribe(
        const std::string &channel,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handler)
{
    assert( pimpl->state == State::Connected ||
            pimpl->state == State::Subscribed);

    static const std::string subscribeStr = "SUBSCRIBE";

    if( pimpl->state == State::Connected || pimpl->state == State::Subscribed )
    {
        Handle handle = {pimpl->subscribeSeq++, channel};

        std::deque<RedisBuffer> items {subscribeStr, channel};

        pimpl->post(std::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items),  std::move(handler)));
        pimpl->msgHandlers.insert(std::make_pair(channel, std::make_pair(handle.id,
                        std::move(msgHandler))));
        pimpl->state = State::Subscribed;

        return handle;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << to_string(pimpl->state);

        pimpl->errorHandler(ss.str());
        return Handle();
    }
}

void RedisAsyncClient::unsubscribe(const Handle &handle)
{
#ifdef DEBUG
    static int recursion = 0;
    assert( recursion++ == 0 );
#endif

    assert( pimpl->state == State::Connected ||
            pimpl->state == State::Subscribed);

    static const std::string unsubscribeStr = "UNSUBSCRIBE";

    if( pimpl->state == State::Connected ||
            pimpl->state == State::Subscribed )
    {
        // Remove subscribe-handler
        typedef RedisClientImpl::MsgHandlersMap::iterator iterator;
        std::pair<iterator, iterator> pair = pimpl->msgHandlers.equal_range(handle.channel);

        for(iterator it = pair.first; it != pair.second;)
        {
            if( it->second.first == handle.id )
            {
                pimpl->msgHandlers.erase(it++);
            }
            else
            {
                ++it;
            }
        }

        std::deque<RedisBuffer> items {unsubscribeStr, handle.channel};

        // Unsubscribe command for Redis
        pimpl->post(std::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), dummyHandler));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << to_string(pimpl->state);

#ifdef DEBUG
        --recursion;
#endif
        pimpl->errorHandler(ss.str());
        return;
    }

#ifdef DEBUG
    --recursion;
#endif
}

void RedisAsyncClient::singleShotSubscribe(const std::string &channel,
                                      std::function<void(std::vector<char> msg)> msgHandler,
                                      std::function<void(RedisValue)> handler)
{
    assert( pimpl->state == State::Connected ||
            pimpl->state == State::Subscribed);

    static const std::string subscribeStr = "SUBSCRIBE";

    if( pimpl->state == State::Connected ||
            pimpl->state == State::Subscribed )
    {
        std::deque<RedisBuffer> items {subscribeStr, channel};

        pimpl->post(std::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), std::move(handler)));
        pimpl->singleShotMsgHandlers.insert(std::make_pair(channel, std::move(msgHandler)));
        pimpl->state = State::Subscribed;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << to_string(pimpl->state);

        pimpl->errorHandler(ss.str());
    }
}


void RedisAsyncClient::publish(const std::string &channel, const RedisBuffer &msg,
                          std::function<void(RedisValue)> handler)
{
    assert( pimpl->state == State::Connected );

    static const std::string publishStr = "PUBLISH";

    if( pimpl->state == State::Connected )
    {
        std::deque<RedisBuffer> items(3);

        items[0] = publishStr;
        items[1] = channel;
        items[2] = msg;

        pimpl->post(std::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), std::move(handler)));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << to_string(pimpl->state);

        pimpl->errorHandler(ss.str());
    }
}

RedisAsyncClient::State RedisAsyncClient::state() const
{
    return pimpl->getState();
}

bool RedisAsyncClient::stateValid() const
{
    assert( pimpl->state == State::Connected );

    if( pimpl->state != State::Connected )
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << to_string(pimpl->state);

        pimpl->errorHandler(ss.str());
        return false;
    }

    return true;
}

}

#endif // REDISASYNCCLIENT_REDISASYNCCLIENT_CPP
