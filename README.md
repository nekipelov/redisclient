redisclient
===========

Build status: [![Build Status](https://travis-ci.org/nekipelov/redisclient.svg?branch=unstable)](https://travis-ci.org/nekipelov/redisclient)

Current version: 0.4.3

Boost.asio based Redis-client header-only library. Simple but powerfull.

Get/set example:

    #include <redisclient/redissyncclient.h>
    
    int main(int, char **)
    {
        boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
        const unsigned short port = 6379;
    
        boost::asio::io_service ioService;
        RedisSyncClient redis(ioService);
        std::string errmsg;

        if( !redis.connect(address, port, errmsg) )
        {
            std::cerr << "Can't connect to redis: " << errmsg << std::endl;
            return EXIT_FAILURE;
        }
    
        RedisValue result;
    
        result = redis.command("SET", "key", "value");
    
        if( result.isError() )
        {
            std::cerr << "SET error: " << result.toString() << "\n";
            return EXIT_FAILURE;
        }
    
        result = redis.command("GET", "key");
    
        if( result.isOk() )
        {
            std::cout << "GET " << key << ": " << result.toString() << "\n";
            return EXIT_SUCCESS;
        }
        else
        {
            std::cerr << "GET error: " << result.toString() << "\n";
            return EXIT_FAILURE;
        }
    }

Async get/set example:

    #include <redisclient/redisasyncclient.h>
    
    static const std::string redisKey = "unique-redis-key-example";
    static const std::string redisValue = "unique-redis-value";
    
    void handleConnected(boost::asio::io_service &ioService, RedisAsyncClient &redis,
            bool ok, const std::string &errmsg)
    {
        if( ok )
        {
            redis.command("SET", redisKey, redisValue, [&](const RedisValue &v) {
                std::cerr << "SET: " << v.toString() << std::endl;

                redis.command("GET", redisKey, [&](const RedisValue &v) {
                    std::cerr << "GET: " << v.toString() << std::endl;
    
                    redis.command("DEL", redisKey, [&](const RedisValue &) {
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
        RedisAsyncClient redis(ioService);

        redis.asyncConnect(address, port,
                boost::bind(&handleConnected, boost::ref(ioService), boost::ref(redis), _1, _2));
    
        ioService.run();
    
        return 0;
    }


    
Publish/subscribe example:

    #include <redisclient/redisasyncclient.h>
    
    static const std::string channelName = "unique-redis-channel-name-example";
    
    void subscribeHandler(boost::asio::io_service &ioService, const std::vector<char> &buf)
    {
        std::string msg(buf.begin(), buf.end());
    
        if( msg == "stop" )
            ioService.stop();
    }
    
    void publishHandler(RedisAsyncClient &publisher, const RedisValue &)
    {
        publisher.publish(channelName, "First hello", [&](const RedisValue &) {
            publisher.publish(channelName, "Last hello", [&](const RedisValue &) {
                publisher.publish(channelName, "stop");
            });
        });
    }
    
    int main(int, char **)
    {
        boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
        const unsigned short port = 6379;
    
        boost::asio::io_service ioService;
        RedisAsyncClient publisher(ioService);
        RedisAsyncClient subscriber(ioService);
    
        publisher.asyncConnect(address, port, [&](bool status, const std::string &err)
        {
            if( !status )
            {
                std::cerr << "Can't connect to to redis" << err << std::endl;
            }
            else
            {
                subscriber.asyncConnect(address, port, [&](bool status, const std::string &err)
                {
                    if( !status )
                    {
                        std::cerr << "Can't connect to to redis" << err << std::endl;
                    }
                    else
                    {
                        subscriber.subscribe(channelName,
                                boost::bind(&subscribeHandler, boost::ref(ioService), _1),
                                boost::bind(&publishHandler, boost::ref(publisher), _1));
                    }
                });
            }
        });
    
        ioService.run();
    
        return 0;
    }


Also you can build the library like a shared library. Just use
 -DREDIS_CLIENT_DYNLIB and -DREDIS_CLIENT_BUILD to build redisclient
and -DREDIS_CLIENT_DYNLIB to build your project.
