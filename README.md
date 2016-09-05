redisclient
===========

Build master status: [![Build travis status](https://travis-ci.org/nekipelov/redisclient.svg?branch=master)](https://travis-ci.org/nekipelov/redisclient)
[![Build appveyor status](https://ci.appveyor.com/api/projects/status/github/nekipelov/redisclient?branch=master)](https://ci.appveyor.com/project/nekipelov/redisclient/branch/master)

Build develop status: [![Build travis status](https://travis-ci.org/nekipelov/redisclient.svg?branch=develop)](https://travis-ci.org/nekipelov/redisclient)
[![Build appveyor status](https://ci.appveyor.com/api/projects/status/github/nekipelov/redisclient?branch=develop)](https://ci.appveyor.com/project/nekipelov/redisclient/branch/develop)

Current version: 0.5.0.

Boost.asio based Redis-client header-only library. Simple but powerfull.

This version requires С++11 compiler. If you want to use this library with old compiler, use version 0.4: https://github.com/nekipelov/redisclient/tree/v0.4.

Get/set example:

```cpp
#include <string>
#include <vector>
#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <redisclient/redissyncclient.h>

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;

    boost::asio::io_service ioService;
    redisclient::RedisSyncClient redis(ioService);
    std::string errmsg;

    if( !redis.connect(address, port, errmsg) )
    {
        std::cerr << "Can't connect to redis: " << errmsg << std::endl;
        return EXIT_FAILURE;
    }

    redisclient::RedisValue result;

    result = redis.command("SET", {"key", "value"});

    if( result.isError() )
    {
        std::cerr << "SET error: " << result.toString() << "\n";
        return EXIT_FAILURE;
    }

    result = redis.command("GET", {"key"});

    if( result.isOk() )
    {
        std::cout << "GET: " << result.toString() << "\n";
        return EXIT_SUCCESS;
    }
    else
    {
        std::cerr << "GET error: " << result.toString() << "\n";
        return EXIT_FAILURE;
    }
}
```

Async get/set example:

```cpp
#include <string>
#include <iostream>
#include <functional>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>

#include <redisclient/redisasyncclient.h>

static const std::string redisKey = "unique-redis-key-example";
static const std::string redisValue = "unique-redis-value";

void handleConnected(boost::asio::io_service &ioService, redisclient::RedisAsyncClient &redis,
        bool ok, const std::string &errmsg)
{
    if( ok )
    {
        redis.command("SET", {redisKey, redisValue}, [&](const redisclient::RedisValue &v) {
            std::cerr << "SET: " << v.toString() << std::endl;

            redis.command("GET", {redisKey}, [&](const redisclient::RedisValue &v) {
                std::cerr << "GET: " << v.toString() << std::endl;

                redis.command("DEL", {redisKey}, [&](const redisclient::RedisValue &) {
                    ioService.stop();
                });
            });
        });
    }
    else
    {
        std::cerr << "Can't connect to redis: " << errmsg << std::endl;
    }
}

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;

    boost::asio::io_service ioService;
    redisclient::RedisAsyncClient redis(ioService);

    redis.asyncConnect(address, port,
            std::bind(&handleConnected, std::ref(ioService), std::ref(redis),
                std::placeholders::_1, std::placeholders::_2));

    ioService.run();

    return 0;
}
```

Publish/subscribe example:

```cpp
#include <string>
#include <iostream>
#include <boost/asio/ip/address.hpp>
#include <boost/format.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <redisclient/redisasyncclient.h>

using namespace redisclient;

static const std::string channelName = "unique-redis-channel-name-example";
static const boost::posix_time::seconds timeout(1);

class Client
{
public:
    Client(boost::asio::io_service &ioService,
           const boost::asio::ip::address &address,
           unsigned short port)
        : ioService(ioService), publishTimer(ioService),
          connectSubscriberTimer(ioService), connectPublisherTimer(ioService),
          address(address), port(port),
          publisher(ioService), subscriber(ioService)
    {
        publisher.installErrorHandler(std::bind(&Client::connectPublisher, this));
        subscriber.installErrorHandler(std::bind(&Client::connectSubscriber, this));
    }

    void publish(const std::string &str)
    {
        publisher.publish(channelName, str);
    }

    void start()
    {
        connectPublisher();
        connectSubscriber();
    }

protected:
    void errorPubProxy(const std::string &)
    {
        publishTimer.cancel();
        connectPublisher();
    }

    void errorSubProxy(const std::string &)
    {
        connectSubscriber();
    }

    void connectPublisher()
    {
        std::cerr << "connectPublisher\n";

        if( publisher.state() == RedisAsyncClient::State::Connected )
        {
            std::cerr << "disconnectPublisher\n";

            publisher.disconnect();
            publishTimer.cancel();
        }

        publisher.connect(address, port,
                          std::bind(&Client::onPublisherConnected, this, std::placeholders::_1, std::placeholders::_2));
    }

    void connectSubscriber()
    {
        std::cerr << "connectSubscriber\n";

        if( subscriber.state() == RedisAsyncClient::State::Connected ||
                subscriber.state() == RedisAsyncClient::State::Subscribed )
        {
            std::cerr << "disconnectSubscriber\n";
            subscriber.disconnect();
        }

        subscriber.connect(address, port,
                           std::bind(&Client::onSubscriberConnected, this, std::placeholders::_1, std::placeholders::_2));
    }

    void callLater(boost::asio::deadline_timer &timer,
                   void(Client::*callback)())
    {
        std::cerr << "callLater\n";
        timer.expires_from_now(timeout);
        timer.async_wait([callback, this](const boost::system::error_code &ec) {
            if( !ec )
            {
                (this->*callback)();
            }
        });
    }

    void onPublishTimeout()
    {
        static size_t counter = 0;
        std::string msg = str(boost::format("message %1%")  % counter++);

        if( publisher.state() == RedisAsyncClient::State::Connected )
        {
            std::cerr << "pub " << msg << "\n";
            publish(msg);
        }

        callLater(publishTimer, &Client::onPublishTimeout);
    }

    void onPublisherConnected(bool status, const std::string &error)
    {
        if( !status )
        {
            std::cerr << "onPublisherConnected: can't connect to redis: " << error << "\n";
            callLater(connectPublisherTimer, &Client::connectPublisher);
        }
        else
        {
            std::cerr << "onPublisherConnected ok\n";

            callLater(publishTimer, &Client::onPublishTimeout);
        }
    }

    void onSubscriberConnected(bool status, const std::string &error)
    {
        if( !status )
        {
            std::cerr << "onSubscriberConnected: can't connect to redis: " << error << "\n";
            callLater(connectSubscriberTimer, &Client::connectSubscriber);
        }
        else
        {
            std::cerr << "onSubscriberConnected ok\n";
            subscriber.subscribe(channelName,
                                 std::bind(&Client::onMessage, this, std::placeholders::_1));
        }
    }

    void onMessage(const std::vector<char> &buf)
    {
        std::string s(buf.begin(), buf.end());
        std::cout << "onMessage: " << s << "\n";
    }

private:
    boost::asio::io_service &ioService;
    boost::asio::deadline_timer publishTimer;
    boost::asio::deadline_timer connectSubscriberTimer;
    boost::asio::deadline_timer connectPublisherTimer;
    const boost::asio::ip::address address;
    const unsigned short port;

    RedisAsyncClient publisher;
    RedisAsyncClient subscriber;
};

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;

    boost::asio::io_service ioService;

    Client client(ioService, address, port);

    client.start();
    ioService.run();

    std::cerr << "done\n";

    return 0;
}
```

Also you can build the library like a shared library. Just use
 -DREDIS_CLIENT_DYNLIB and -DREDIS_CLIENT_BUILD to build redisclient
and -DREDIS_CLIENT_DYNLIB to build your project.
