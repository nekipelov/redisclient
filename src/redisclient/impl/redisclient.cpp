/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISCLIENT_CPP
#define REDISCLIENT_REDISCLIENT_CPP

#include <boost/make_shared.hpp>
#include "../redisclient.h"

static const std::string subscribeStr = "SUBSCRIBE";

RedisClient::RedisClient(boost::asio::io_service &ioService)
    : pimpl(boost::make_shared<RedisClientImpl>(boost::ref(ioService)))
{
    pimpl->errorHandler = boost::bind(&RedisClientImpl::defaulErrorHandler,
                                      pimpl, _1);
}

RedisClient::~RedisClient()
{
    pimpl->close();
}

bool RedisClient::connect(const boost::asio::ip::address &address,
                          unsigned short port)
{
    boost::asio::ip::tcp::endpoint endpoint(address, port);
    boost::system::error_code ec;

    pimpl->socket.connect(endpoint, ec);

    if( !ec )
    {
        pimpl->state = RedisClientImpl::Connected;
        pimpl->processMessage();
        return true;
    }
    else
    {
        return false;
    }
}

void RedisClient::asyncConnect(const boost::asio::ip::address &address,
                               unsigned short port,
                               const boost::function<void(bool, const std::string &)> &handler)
{
    boost::asio::ip::tcp::endpoint endpoint(address, port);
    asyncConnect(endpoint, handler);
}

void RedisClient::asyncConnect(const boost::asio::ip::tcp::endpoint &endpoint,
                               const boost::function<void(bool, const std::string &)> &handler)
{
    pimpl->socket.async_connect(endpoint, boost::bind(&RedisClientImpl::handleAsyncConnect,
                                                      pimpl, _1, handler));
}


void RedisClient::installErrorHandler(
        const boost::function<void(const std::string &)> &handler)
{
    pimpl->errorHandler = handler;
}

void RedisClient::command(const std::string &s, const boost::function<void(const RedisValue &)> &handler)
{
    if(stateValid())
    {
        std::vector<std::string> items(1);
        items[0] = s;

        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, handler));
    }
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const boost::function<void(const RedisValue &)> &handler)
{
    if(stateValid())
    {
        std::vector<std::string> items(2);
        items[0] = cmd;
        items[1] = arg1;

        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, handler));
    }
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const std::string &arg2,
                          const boost::function<void(const RedisValue &)> &handler)
{
    if(stateValid())
    {
        std::vector<std::string> items(3);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;

        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, handler));
    }
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const std::string &arg2, const std::string &arg3,
                          const boost::function<void(const RedisValue &)> &handler)
{
    if(stateValid())
    {
        std::vector<std::string> items(4);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;
        items[3] = arg3;

        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, handler));
    }
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const std::string &arg2, const std::string &arg3,
                          const std::string &arg4,
                          const boost::function<void(const RedisValue &)> &handler)
{
    if(stateValid())
    {
        std::vector<std::string> items(5);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;
        items[3] = arg3;
        items[4] = arg4;

        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, handler));
    }
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const std::string &arg2, const std::string &arg3,
                          const std::string &arg4, const std::string &arg5,
                          const boost::function<void(const RedisValue &)> &handler)
{
    if(stateValid())
    {
        std::vector<std::string> items(6);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;
        items[3] = arg3;
        items[4] = arg4;
        items[5] = arg5;

        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, handler));
    }
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const std::string &arg2, const std::string &arg3,
                          const std::string &arg4, const std::string &arg5,
                          const std::string &arg6,
                          const boost::function<void(const RedisValue &)> &handler)
{
    if(stateValid())
    {
        std::vector<std::string> items(7);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;
        items[3] = arg3;
        items[4] = arg4;
        items[5] = arg5;
        items[6] = arg6;

        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, handler));
    }
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const std::string &arg2, const std::string &arg3,
                          const std::string &arg4, const std::string &arg5,
                          const std::string &arg6, const std::string &arg7,
                          const boost::function<void(const RedisValue &)> &handler)
{
    if(stateValid())
    {
        std::vector<std::string> items(8);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;
        items[3] = arg3;
        items[4] = arg4;
        items[5] = arg5;
        items[6] = arg6;
        items[7] = arg7;

        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, handler));
    }
}

void RedisClient::command(const std::string &cmd, const std::list<std::string> &args,
                          const boost::function<void(const RedisValue &)> &handler)
{
    if(stateValid())
    {
        std::vector<std::string> items(1);
        items[0] = cmd;

        items.reserve(1 + args.size());

        std::copy(args.begin(), args.end(), std::back_inserter(items));
        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, handler));
    }
}

RedisClient::Handle RedisClient::subscribe(
        const std::string &channel,
        const boost::function<void(const std::string &msg)> &msgHandler,
        const boost::function<void(const RedisValue &)> &handler)
{
    assert( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed);

    if( pimpl->state == RedisClientImpl::Connected || pimpl->state == RedisClientImpl::Subscribed )
    {
        Handle handle = {pimpl->subscribeSeq++, channel};

        std::vector<std::string> items(2);
        items[0] = subscribeStr;
        items[1] = channel;

        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, handler));
        pimpl->msgHandlers.insert(std::make_pair(channel, std::make_pair(handle.id, msgHandler)));
        pimpl->state = RedisClientImpl::Subscribed;

        return handle;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << pimpl->state;

        pimpl->onError(ss.str());
        return Handle();
    }
}

void RedisClient::unsubscribe(const Handle &handle)
{
#ifdef DEBUG
    static int recursion = 0;
    assert( recursion++ == 0 );
#endif

    assert( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed);

    static const std::string unsubscribeStr = "UNSUBSCRIBE";

    if( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed )
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

        std::vector<std::string> items(2);
        items[0] = unsubscribeStr;
        items[1] = handle.channel;

        // Unsubscribe command for Redis
        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, dummyHandler));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << pimpl->state;

#ifdef DEBUG
        --recursion;
#endif
        pimpl->onError(ss.str());
        return;
    }

#ifdef DEBUG
    --recursion;
#endif
}

void RedisClient::singleShotSubscribe(const std::string &channel,
                                      const boost::function<void(const std::string &msg)> &msgHandler,
                                      const boost::function<void(const RedisValue &)> &handler)
{
    assert( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed);

    if( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed )
    {
        std::vector<std::string> items(2);
        items[0] = subscribeStr;
        items[1] = channel;

        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, handler));
        pimpl->singleShotMsgHandlers.insert(std::make_pair(channel, msgHandler));
        pimpl->state = RedisClientImpl::Subscribed;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << pimpl->state;

        pimpl->onError(ss.str());
    }
}


void RedisClient::publish(const std::string &channel, const std::string &msg,
                          const boost::function<void(const RedisValue &)> &handler)
{
    assert( pimpl->state == RedisClientImpl::Connected );

    static const std::string publishStr = "PUBLISH";

    if( pimpl->state == RedisClientImpl::Connected )
    {
        std::vector<std::string> items(3);

        items[0] = publishStr;
        items[1] = channel;
        items[2] = msg;

        pimpl->post(boost::bind(&RedisClientImpl::doCommand, pimpl, items, handler));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << pimpl->state;

        pimpl->onError(ss.str());
    }
}

bool RedisClient::stateValid() const
{
    assert( pimpl->state == RedisClientImpl::Connected );

    if( pimpl->state != RedisClientImpl::Connected )
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << pimpl->state;

        pimpl->onError(ss.str());
        return false;
    }

    return true;
}

#endif // REDISCLIENT_REDISCLIENT_CPP
