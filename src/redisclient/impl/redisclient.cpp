/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISCLIENT_CPP
#define REDISCLIENT_REDISCLIENT_CPP

#include "../redisclient.h"

RedisClient::RedisClient(boost::asio::io_service &ioService)
    : impl(ioService)
{
    impl.errorHandler = boost::bind(&RedisClientImpl::defaulErrorHandler,
                                    &impl, _1);
}

RedisClient::~RedisClient()
{
}

bool RedisClient::connect(const char *address, int port)
{
    boost::asio::ip::tcp::resolver resolver(impl.strand.get_io_service());
    boost::asio::ip::tcp::resolver::query query(address,
                                                boost::lexical_cast<std::string>(port));

    boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);

    while(it != boost::asio::ip::tcp::resolver::iterator())
    {
        boost::asio::ip::tcp::endpoint endpoint = *it;
        boost::system::error_code ec;

        impl.socket.connect(endpoint, ec);

        if( !ec )
        {
            impl.state = RedisClientImpl::Connected;
            impl.processMessage();
            return true;
        }
        else
        {
            ++it;
        }
    }

    return false;
}

void RedisClient::asyncConnect(const boost::asio::ip::tcp::endpoint &endpoint,
                               const boost::function<void(const boost::system::error_code &)> &handler)
{
    impl.socket.async_connect(endpoint, boost::bind(&RedisClientImpl::handleAsyncConnect,
                                                    &impl, _1, handler));
}

void RedisClient::installErrorHandler(
        const boost::function<void(const std::string &)> &handler)
{
    impl.errorHandler = handler;
}

void RedisClient::command(const std::string &s, const boost::function<void(const RedisValue &)> &handler)
{
    checkState();

    std::vector<std::string> items(1);
    items[0] = s;

    impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, handler));
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const boost::function<void(const RedisValue &)> &handler)
{
    checkState();

    std::vector<std::string> items(2);
    items[0] = cmd;
    items[1] = arg1;

    impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, handler));
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const std::string &arg2,
                          const boost::function<void(const RedisValue &)> &handler)
{
    checkState();

    std::vector<std::string> items(3);
    items[0] = cmd;
    items[1] = arg1;
    items[2] = arg2;

    impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, handler));
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const std::string &arg2, const std::string &arg3,
                          const boost::function<void(const RedisValue &)> &handler)
{
    checkState();

    std::vector<std::string> items(4);
    items[0] = cmd;
    items[1] = arg1;
    items[2] = arg2;
    items[3] = arg3;

    impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, handler));
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const std::string &arg2, const std::string &arg3,
                          const std::string &arg4,
                          const boost::function<void(const RedisValue &)> &handler)
{
    checkState();

    std::vector<std::string> items(5);
    items[0] = cmd;
    items[1] = arg1;
    items[2] = arg2;
    items[3] = arg3;
    items[4] = arg4;

    impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, handler));
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const std::string &arg2, const std::string &arg3,
                          const std::string &arg4, const std::string &arg5,
                          const boost::function<void(const RedisValue &)> &handler)
{
    checkState();

    std::vector<std::string> items(6);
    items[0] = cmd;
    items[1] = arg1;
    items[2] = arg2;
    items[3] = arg3;
    items[4] = arg4;
    items[5] = arg5;

    impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, handler));
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const std::string &arg2, const std::string &arg3,
                          const std::string &arg4, const std::string &arg5,
                          const std::string &arg6,
                          const boost::function<void(const RedisValue &)> &handler)
{
    checkState();

    std::vector<std::string> items(7);
    items[0] = cmd;
    items[1] = arg1;
    items[2] = arg2;
    items[3] = arg3;
    items[4] = arg4;
    items[5] = arg5;
    items[6] = arg6;

    impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, handler));
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const std::string &arg2, const std::string &arg3,
                          const std::string &arg4, const std::string &arg5,
                          const std::string &arg6, const std::string &arg7,
                          const boost::function<void(const RedisValue &)> &handler)
{
    checkState();

    std::vector<std::string> items(8);
    items[0] = cmd;
    items[1] = arg1;
    items[2] = arg2;
    items[3] = arg3;
    items[4] = arg4;
    items[5] = arg5;
    items[6] = arg6;
    items[7] = arg7;

    impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, handler));
}

void RedisClient::command(const std::string &cmd, const std::list<std::string> &args,
                          const boost::function<void(const RedisValue &)> &handler)
{
    checkState();

    std::vector<std::string> items(1);
    items[0] = cmd;

    items.reserve(1 + args.size());

    std::copy(args.begin(), args.end(), std::back_inserter(items));
    impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, handler));
}

RedisClient::Handle RedisClient::subscribe(
        const std::string &channel,
        const boost::function<void(const std::string &msg)> &msgHandler,
        const boost::function<void(const RedisValue &)> &handler)
{
    assert( impl.state == RedisClientImpl::Connected ||
            impl.state == RedisClientImpl::Subscribed);

    static std::string subscribe = "SUBSCRIBE";

    if( impl.state == RedisClientImpl::Connected || impl.state == RedisClientImpl::Subscribed )
    {
        Handle handle = {impl.subscribeSeq++, channel};

        std::vector<std::string> items(2);
        items[0] = subscribe;
        items[1] = channel;

        impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, handler));
        impl.msgHandlers.insert(std::make_pair(channel, std::make_pair(handle.id, msgHandler)));
        impl.state = RedisClientImpl::Subscribed;

        return handle;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << impl.state;

        impl.errorHandler(ss.str());
        return Handle();
    }
}

void RedisClient::unsubscribe(const Handle &handle)
{
#ifdef DEBUG
    static int recursion = 0;
    assert( recursion++ == 0 );
#endif

    assert( impl.state == RedisClientImpl::Connected ||
            impl.state == RedisClientImpl::Subscribed);

    static std::string unsubscribe = "UNSUBSCRIBE";

    if( impl.state == RedisClientImpl::Connected ||
            impl.state == RedisClientImpl::Subscribed )
    {
        // Remove subscribe-handler
        typedef RedisClientImpl::MsgHandlersMap::iterator iterator;
        std::pair<iterator, iterator> pair = impl.msgHandlers.equal_range(handle.channel);

        for(iterator it = pair.first; it != pair.second;)
        {
            if( it->second.first == handle.id )
            {
                impl.msgHandlers.erase(it++);
            }
            else
            {
                ++it;
            }
        }

        std::vector<std::string> items(2);
        items[0] = unsubscribe;
        items[1] = handle.channel;

        // Unsubscribe command for Redis
        impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, dummyHandler));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << impl.state;

#ifdef DEBUG
        --recursion;
#endif
        impl.errorHandler(ss.str());
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
    assert( impl.state == RedisClientImpl::Connected ||
            impl.state == RedisClientImpl::Subscribed);

    static std::string subscribe = "SUBSCRIBE";

    if( impl.state == RedisClientImpl::Connected ||
            impl.state == RedisClientImpl::Subscribed )
    {
        std::vector<std::string> items(2);
        items[0] = subscribe;
        items[1] = channel;

        impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, handler));
        impl.singleShotMsgHandlers.insert(std::make_pair(channel, msgHandler));
        impl.state = RedisClientImpl::Subscribed;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << impl.state;

        impl.errorHandler(ss.str());
    }
}


void RedisClient::publish(const std::string &channel, const std::string &msg,
                          const boost::function<void(const RedisValue &)> &handler)
{
    assert( impl.state == RedisClientImpl::Connected );

    static std::string publish = "PUBLISH";

    if( impl.state == RedisClientImpl::Connected )
    {
        std::vector<std::string> items(3);

        items[0] = publish;
        items[1] = channel;
        items[2] = msg;

        impl.post(boost::bind(&RedisClientImpl::doCommand, &impl, items, handler));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << impl.state;

        impl.errorHandler(ss.str());
    }
}

void RedisClient::checkState() const
{
    assert( impl.state == RedisClientImpl::Connected );

    if( impl.state != RedisClientImpl::Connected )
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << impl.state;

        impl.errorHandler(ss.str());
    }
}

#endif // REDISCLIENT_REDISCLIENT_CPP
