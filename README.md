redisclient
===========

Build master status: [![Build travis status](https://travis-ci.org/nekipelov/redisclient.svg?branch=master)](https://travis-ci.org/nekipelov/redisclient)
[![Build appveyor status](https://ci.appveyor.com/api/projects/status/github/nekipelov/redisclient?branch=master)](https://ci.appveyor.com/project/nekipelov/redisclient/branch/master)

Build develop status: [![Build travis status](https://travis-ci.org/nekipelov/redisclient.svg?branch=develop)](https://travis-ci.org/nekipelov/redisclient)
[![Build appveyor status](https://ci.appveyor.com/api/projects/status/github/nekipelov/redisclient?branch=develop)](https://ci.appveyor.com/project/nekipelov/redisclient/branch/develop)

Current version: 1.0.0.

Boost.asio based Redis-client header-only library. Simple but powerfull.

This version requires С++11 compiler. If you want to use this library with old compiler, use version 0.4: https://github.com/nekipelov/redisclient/tree/v0.4.

Have amalgamated sources. See files in the `amalgamated` directory.

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
    boost::asio::ip::tcp::endpoint endpoint(address, port);

    boost::asio::io_service ioService;
    redisclient::RedisSyncClient redis(ioService);
    boost::system::error_code ec;

    redis.connect(endpoint, ec);

    if(ec)
    {
        std::cerr << "Can't connect to redis: " << ec.message() << std::endl;
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
        boost::system::error_code ec)
{
    if( !ec )
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
        std::cerr << "Can't connect to redis: " << ec.message() << std::endl;
    }
}

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;
    boost::asio::ip::tcp::endpoint endpoint(address, port);

    boost::asio::io_service ioService;
    redisclient::RedisAsyncClient redis(ioService);

    redis.connect(endpoint,
            std::bind(&handleConnected, std::ref(ioService), std::ref(redis),
                std::placeholders::_1));

    ioService.run();

    return 0;
}
```

Also you can build the library like a shared library. Just use
 -DREDIS_CLIENT_DYNLIB and -DREDIS_CLIENT_BUILD to build redisclient
and -DREDIS_CLIENT_DYNLIB to build your project.
