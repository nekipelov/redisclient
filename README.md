redisclient
===========

Build status: [![Build Status](https://travis-ci.org/nekipelov/redisclient.svg?branch=unstable)](https://travis-ci.org/nekipelov/redisclient)

Current version: 0.4.0

Boost.asio based Redis-client header-only library. Simple but powerfull.

Get/set example:

    #include "redisclient.h"
    
    static const std::string redisKey = "unique-redis-key-example";
    static const std::string redisValue = "unique-redis-value";
    
    int main(int, char **)
    {
        const char *address = "127.0.0.1";
        const int port = 6379;
    
        boost::asio::io_service ioService;
        RedisClient redis(ioService);
    
        if( !redis.connect(address, port) )
        {
            std::cerr << "Can't connecto to redis" << std::endl;
            return EXIT_FAILURE;
        }
    
        redis.command("SET", redisKey, redisValue, [&](const Value &v) {
            std::cerr << "SET: " << v.toString() << std::endl;
    
            redis.command("GET", redisKey, [&](const Value &v) {
                std::cerr << "GET: " << v.toString() << std::endl;
    
                redis.command("DEL", redisKey, [&](const Value &v) {
                    ioService.stop();
                });
            });
        });
    
        ioService.run();
    
        return 0;
    }

    
Publish/subscribe example:

    #include "redisclient.h"
    
    static const std::string channelName = "unique-redis-channel-name-example";
    
    int main(int, char **)
    {
        const char *address = "127.0.0.1";
        const int port = 6379;
    
        boost::asio::io_service ioService;
        RedisClient publisher(ioService);
        RedisClient subscriber(ioService);
    
        if( !publisher.connect(address, port) || !subscriber.connect(address, port) )
        {
            std::cerr << "Can't connecto to redis" << std::endl;
            return EXIT_FAILURE;
        }
    
        subscriber.subscribe(channelName, [&](const std::string &msg) {
            std::cerr << "Message: " << msg << std::endl;
    
            if( msg == "stop" )
                ioService.stop();
        });
    
        publisher.publish(channelName, "First hello", [&](const Value &) {
            publisher.publish(channelName, "Last hello", [&](const Value &) {
                publisher.publish(channelName, "stop");
            });
        });
    
        ioService.run();
    
        return 0;
    }

Also you can build the library like a shared library. Just use
 -DREDIS_CLIENT_DYNLIB and -DREDIS_CLIENT_BUILD to build redisclient
and -DREDIS_CLIENT_DYNLIB to build your project.
