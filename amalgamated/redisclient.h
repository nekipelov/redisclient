
/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */


#define REDISCLIENT_VERSION_H

#define REDIS_CLIENT_VERSION 600 // 0.6.0


/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */



#define REDISSYNCCLIENT_REDISBUFFER_H

#include <boost/variant.hpp>

#include <string>
#include <vector>



namespace redisclient {

struct RedisBuffer
{
    RedisBuffer() = default;
    inline RedisBuffer(const char *ptr, size_t dataSize);
    inline RedisBuffer(const char *s);
    inline RedisBuffer(std::string s);
    inline RedisBuffer(std::vector<char> buf);

    inline size_t size() const;

    boost::variant<std::string,std::vector<char>> data;
};


RedisBuffer::RedisBuffer(const char *ptr, size_t dataSize)
    : data(std::vector<char>(ptr, ptr + dataSize))
{
}

RedisBuffer::RedisBuffer(const char *s)
    : data(std::string(s))
{
}

RedisBuffer::RedisBuffer(std::string s)
    : data(std::move(s))
{
}

RedisBuffer::RedisBuffer(std::vector<char> buf)
    : data(std::move(buf))
{
}

size_t RedisBuffer::size() const
{
    if (data.type() == typeid(std::string))
        return boost::get<std::string>(data).size();
    else
        return boost::get<std::vector<char>>(data).size();
}

}



/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */


#define REDISCLIENT_REDISVALUE_H

#include <boost/variant.hpp>
#include <string>
#include <vector>



namespace redisclient {

class RedisValue {
public:
    struct ErrorTag {};

     RedisValue();
     RedisValue(RedisValue &&other);
     RedisValue(int64_t i);
     RedisValue(const char *s);
     RedisValue(const std::string &s);
     RedisValue(std::vector<char> buf);
     RedisValue(std::vector<char> buf, struct ErrorTag);
     RedisValue(std::vector<RedisValue> array);


    RedisValue(const RedisValue &) = default;
    RedisValue& operator = (const RedisValue &) = default;
    RedisValue& operator = (RedisValue &&) = default;

    // Return the value as a std::string if
    // type is a byte string; otherwise returns an empty std::string.
     std::string toString() const;

    // Return the value as a std::vector<char> if
    // type is a byte string; otherwise returns an empty std::vector<char>.
     std::vector<char> toByteArray() const;

    // Return the value as a std::vector<RedisValue> if
    // type is an int; otherwise returns 0.
     int64_t toInt() const;

    // Return the value as an array if type is an array;
    // otherwise returns an empty array.
     std::vector<RedisValue> toArray() const;

    // Return the string representation of the value. Use
    // for dump content of the value.
     std::string inspect() const;

    // Return true if value not a error
     bool isOk() const;
    // Return true if value is a error
     bool isError() const;


    // Return true if this is a null.
     bool isNull() const;
    // Return true if type is an int
     bool isInt() const;
    // Return true if type is an array
     bool isArray() const;
    // Return true if type is a string/byte array. Alias for isString();
     bool isByteArray() const;
    // Return true if type is a string/byte array. Alias for isByteArray().
     bool isString() const;

     bool operator == (const RedisValue &rhs) const;
     bool operator != (const RedisValue &rhs) const;

     std::vector<RedisValue> &getArray();
protected:
    template<typename T>
     T castTo() const;

    template<typename T>
    bool typeEq() const;

private:
    struct NullTag {
        inline bool operator == (const NullTag &) const {
            return true;
        }
    };


    boost::variant<NullTag, int64_t, std::vector<char>, std::vector<RedisValue> > value;
    bool error;
};


template<typename T>
T RedisValue::castTo() const
{
    if( value.type() == typeid(T) )
        return boost::get<T>(value);
    else
        return T();
}

template<typename T>
bool RedisValue::typeEq() const
{
    if( value.type() == typeid(T) )
        return true;
    else
        return false;
}

}






/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */


#define REDISCLIENT_REDISPARSER_H

#include <stack>
#include <vector>
#include <utility>




namespace redisclient {

class RedisParser
{
public:
     RedisParser();

    enum ParseResult {
        Completed,
        Incompleted,
        Error,
    };

     std::pair<size_t, ParseResult> parse(const char *ptr, size_t size);

     RedisValue result();

protected:
     std::pair<size_t, ParseResult> parseChunk(const char *ptr, size_t size);

    inline bool isChar(int c)
    {
        return c >= 0 && c <= 127;
    }

    inline bool isControl(int c)
    {
        return (c >= 0 && c <= 31) || (c == 127);
    }

     long int bufToLong(const char *str, size_t size);

private:
    enum State {
        Start = 0,
        StartArray = 1,

        String = 2,
        StringLF = 3,

        ErrorString = 4,
        ErrorLF = 5,

        Integer = 6,
        IntegerLF = 7,

        BulkSize = 8,
        BulkSizeLF = 9,
        Bulk = 10,
        BulkCR = 11,
        BulkLF = 12,

        ArraySize = 13,
        ArraySizeLF = 14,
    };

    std::stack<State> states;

    long int bulkSize;
    std::vector<char> buf;
    RedisValue redisValue;

    // temporary variables
    std::stack<long int> arraySizes;
    std::stack<RedisValue> arrayValues;

    static const char stringReply = '+';
    static const char errorReply = '-';
    static const char integerReply = ':';
    static const char bulkReply = '$';
    static const char arrayReply = '*';
};

}






/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */


#define REDISCLIENT_REDISCLIENTIMPL_H

#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio/generic/stream_protocol.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/strand.hpp>

#include <string>
#include <vector>
#include <queue>
#include <map>
#include <functional>
#include <memory>





namespace redisclient {

class RedisClientImpl : public std::enable_shared_from_this<RedisClientImpl> {
public:
    enum class State {
        Unconnected,
        Connecting,
        Connected,
        Subscribed,
        Closed
    };

     RedisClientImpl(boost::asio::io_service &ioService);
     ~RedisClientImpl();

     void handleAsyncConnect(
            const boost::system::error_code &ec,
            std::function<void(boost::system::error_code)> handler);

     size_t subscribe(const std::string &command,
        const std::string &channel,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handler);

     void singleShotSubscribe(const std::string &command,
        const std::string &channel,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handler);

     void unsubscribe(const std::string &command, 
        size_t handle_id, const std::string &channel, 
        std::function<void(RedisValue)> handler);

     void close() noexcept;

     State getState() const;

     static std::vector<char> makeCommand(const std::deque<RedisBuffer> &items);

     RedisValue doSyncCommand(const std::deque<RedisBuffer> &command,
        const boost::posix_time::time_duration &timeout,
        boost::system::error_code &ec);
     RedisValue doSyncCommand(const std::deque<std::deque<RedisBuffer>> &commands,
        const boost::posix_time::time_duration &timeout,
        boost::system::error_code &ec);
     RedisValue syncReadResponse(
            const boost::posix_time::time_duration &timeout,
            boost::system::error_code &ec);

     void doAsyncCommand(
            std::vector<char> buff,
            std::function<void(RedisValue)> handler);

     void sendNextCommand();
     void processMessage();
     void doProcessMessage(RedisValue v);
     void asyncWrite(const boost::system::error_code &ec, const size_t);
     void asyncRead(const boost::system::error_code &ec, const size_t);

     void onRedisError(const RedisValue &);
     static void defaulErrorHandler(const std::string &s);

    template<typename Handler>
    inline void post(const Handler &handler);

    boost::asio::io_service &ioService;
    boost::asio::strand strand;
    boost::asio::generic::stream_protocol::socket socket;
    RedisParser redisParser;
    boost::array<char, 4096> buf;
    size_t bufSize; // only for sync
    size_t subscribeSeq;

    typedef std::pair<size_t, std::function<void(const std::vector<char> &buf)> > MsgHandlerType;
    typedef std::function<void(const std::vector<char> &buf)> SingleShotHandlerType;

    typedef std::multimap<std::string, MsgHandlerType> MsgHandlersMap;
    typedef std::multimap<std::string, SingleShotHandlerType> SingleShotHandlersMap;

    std::queue<std::function<void(RedisValue)> > handlers;
    std::deque<std::vector<char>> dataWrited;
    std::deque<std::vector<char>> dataQueued;
    MsgHandlersMap msgHandlers;
    SingleShotHandlersMap singleShotMsgHandlers;

    std::function<void(const std::string &)> errorHandler;
    State state;
};

template<typename Handler>
inline void RedisClientImpl::post(const Handler &handler)
{
    strand.post(handler);
}

inline std::string to_string(RedisClientImpl::State state)
{
    switch(state)
    {
        case RedisClientImpl::State::Unconnected:
            return "Unconnected";
            break;
        case RedisClientImpl::State::Connecting:
            return "Connecting";
            break;
        case RedisClientImpl::State::Connected:
            return "Connected";
            break;
        case RedisClientImpl::State::Subscribed:
            return "Subscribed";
            break;
        case RedisClientImpl::State::Closed:
            return "Closed";
            break;
    }

    return "Invalid";
}
}







#pragma once

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

namespace redisclient
{

namespace detail
{

inline void throwError(const boost::system::error_code &ec)
{
    boost::system::system_error error(ec);
    throw error;
}

inline void throwIfError(const boost::system::error_code &ec)
{
    if (ec)
    {
        throwError(ec);
    }
}

}

}
/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */


#define REDISASYNCCLIENT_REDISCLIENT_H

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <list>
#include <type_traits>
#include <functional>






namespace redisclient {

class RedisClientImpl;

class RedisAsyncClient : boost::noncopyable {
public:
    // Subscribe handle.
    struct Handle {
        size_t id;
        std::string channel;
    };

    typedef RedisClientImpl::State State;

     RedisAsyncClient(boost::asio::io_service &ioService);
     ~RedisAsyncClient();

    // Connect to redis server
     void connect(
            const boost::asio::ip::tcp::endpoint &endpoint,
            std::function<void(boost::system::error_code)> handler);


     void connect(
            const boost::asio::local::stream_protocol::endpoint &endpoint,
            std::function<void(boost::system::error_code)> handler);


    // Return true if is connected to redis.
     bool isConnected() const;

    // Return connection state. See RedisClientImpl::State.
     State state() const;

    // disconnect from redis and clear command queue
     void disconnect();

    // Set custom error handler.
     void installErrorHandler(
            std::function<void(const std::string &)> handler);

    // Execute command on Redis server with the list of arguments.
     void command(
            const std::string &cmd, std::deque<RedisBuffer> args,
            std::function<void(RedisValue)> handler = dummyHandler);

    // Subscribe to channel. Handler msgHandler will be called
    // when someone publish message on channel. Call unsubscribe 
    // to stop the subscription.
     Handle subscribe(const std::string &channelName,
                                       std::function<void(std::vector<char> msg)> msgHandler,
                                       std::function<void(RedisValue)> handler = &dummyHandler);


     Handle psubscribe(const std::string &pattern,
                                        std::function<void(std::vector<char> msg)> msgHandler,
                                        std::function<void(RedisValue)> handler = &dummyHandler);

    // Unsubscribe
     void unsubscribe(const Handle &handle);
     void punsubscribe(const Handle &handle);

    // Subscribe to channel. Handler msgHandler will be called
    // when someone publish message on channel; it will be 
    // unsubscribed after call.
     void singleShotSubscribe(
            const std::string &channel,
            std::function<void(std::vector<char> msg)> msgHandler,
            std::function<void(RedisValue)> handler = &dummyHandler);

     void singleShotPSubscribe(
            const std::string &channel,
            std::function<void(std::vector<char> msg)> msgHandler,
            std::function<void(RedisValue)> handler = &dummyHandler);

    // Publish message on channel.
     void publish(
            const std::string &channel, const RedisBuffer &msg,
            std::function<void(RedisValue)> handler = &dummyHandler);

     static void dummyHandler(RedisValue) {}

protected:
     bool stateValid() const;

private:
    std::shared_ptr<RedisClientImpl> pimpl;
};

}






/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */


#define REDISSYNCCLIENT_REDISCLIENT_H

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <list>
#include <functional>






namespace redisclient {

class RedisClientImpl;
class Pipeline;

class RedisSyncClient : boost::noncopyable {
public:
    typedef RedisClientImpl::State State;

     RedisSyncClient(boost::asio::io_service &ioService);
     RedisSyncClient(RedisSyncClient &&other);
     ~RedisSyncClient();

    // Connect to redis server
     void connect(
            const boost::asio::ip::tcp::endpoint &endpoint,
            boost::system::error_code &ec);

    // Connect to redis server
     void connect(
            const boost::asio::ip::tcp::endpoint &endpoint);


     void connect(
            const boost::asio::local::stream_protocol::endpoint &endpoint,
            boost::system::error_code &ec);

     void connect(
            const boost::asio::local::stream_protocol::endpoint &endpoint);


    // Return true if is connected to redis.
     bool isConnected() const;

    // disconnect from redis
     void disconnect();

    // Set custom error handler.
     void installErrorHandler(
        std::function<void(const std::string &)> handler);

    // Execute command on Redis server with the list of arguments.
     RedisValue command(
            std::string cmd, std::deque<RedisBuffer> args);

    // Execute command on Redis server with the list of arguments.
     RedisValue command(
            std::string cmd, std::deque<RedisBuffer> args,
            boost::system::error_code &ec);

    // Create pipeline (see Pipeline)
     Pipeline pipelined();

     RedisValue pipelined(
            std::deque<std::deque<RedisBuffer>> commands,
            boost::system::error_code &ec);

     RedisValue pipelined(
            std::deque<std::deque<RedisBuffer>> commands);

    // Return connection state. See RedisClientImpl::State.
     State state() const;

     RedisSyncClient &setConnectTimeout(
            const boost::posix_time::time_duration &timeout);
     RedisSyncClient &setCommandTimeout(
            const boost::posix_time::time_duration &timeout);

     RedisSyncClient &setTcpNoDelay(bool enable);
     RedisSyncClient &setTcpKeepAlive(bool enable);

protected:
     bool stateValid() const;

private:
    std::shared_ptr<RedisClientImpl> pimpl;
    boost::posix_time::time_duration connectTimeout;
    boost::posix_time::time_duration commandTimeout;
    bool tcpNoDelay;
    bool tcpKeepAlive;
};

}






/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#pragma once

#include <deque>




namespace redisclient
{

class RedisSyncClient;
class RedisValue;

// See https://redis.io/topics/pipelining.
class Pipeline
{
public:
     Pipeline(RedisSyncClient &client);

    // add command to pipe
     Pipeline &command(std::string cmd, std::deque<RedisBuffer> args);

    // Sends all commands to the redis server.
    // For every request command will get response value.
    // Example:
    //
    //  Pipeline pipe(redis);
    //
    //  pipe.command("GET", {"foo"})
    //      .command("GET", {"bar"})
    //      .command("GET", {"more"});
    //
    //  std::vector<RedisValue> result = pipe.finish().toArray();
    //
    //  result[0];  // value of the key "foo"
    //  result[1];  // value of the key "bar"
    //  result[2];  // value of the key "more"
    //
     RedisValue finish();

private:
    std::deque<std::deque<RedisBuffer>> commands;
    RedisSyncClient &client;
};

}





