
/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */


#define REDISCLIENT_VERSION_H

#define REDIS_CLIENT_VERSION 501 // 0.5.1


/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */



#define REDISSYNCCLIENT_REDISBUFFER_H

#include <boost/noncopyable.hpp>

#include <string>
#include <list>
#include <vector>
#include <string.h>



namespace redisclient {

class RedisBuffer
{
public:
    inline RedisBuffer();
    inline RedisBuffer(const char *ptr, size_t dataSize);
    inline RedisBuffer(const char *s);
    inline RedisBuffer(const std::string &s);
    inline RedisBuffer(const std::vector<char> &buf);

    inline size_t size() const;
    inline const char *data() const;

private:
    const char *ptr_;
    size_t size_;
};


RedisBuffer::RedisBuffer()
    : ptr_(NULL), size_(0)
{
}

RedisBuffer::RedisBuffer(const char *ptr, size_t dataSize)
    : ptr_(ptr), size_(dataSize)
{
}

RedisBuffer::RedisBuffer(const char *s)
    : ptr_(s), size_(s == NULL ? 0 : strlen(s))
{
}

RedisBuffer::RedisBuffer(const std::string &s)
    : ptr_(s.c_str()), size_(s.length())
{
}

RedisBuffer::RedisBuffer(const std::vector<char> &buf)
    : ptr_(buf.empty() ? NULL : &buf[0]), size_(buf.size())
{
}

size_t RedisBuffer::size() const
{
    return size_;
}

const char *RedisBuffer::data() const
{
    return ptr_;
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
     std::pair<size_t, ParseResult> parseArray(const char *ptr, size_t size);

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

        String = 1,
        StringLF = 2,

        ErrorString = 3,
        ErrorLF = 4,

        Integer = 5,
        IntegerLF = 6,

        BulkSize = 7,
        BulkSizeLF = 8,
        Bulk = 9,
        BulkCR = 10,
        BulkLF = 11,

        ArraySize = 12,
        ArraySizeLF = 13,

    } state;

    long int bulkSize;
    std::vector<char> buf;
    std::stack<long int> arrayStack;
    std::stack<RedisValue> valueStack;

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
            const std::function<void(bool, const std::string &)> &handler);

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

     RedisValue doSyncCommand(const std::deque<RedisBuffer> &buff);

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

     static void append(std::vector<char> &vec, const RedisBuffer &buf);
     static void append(std::vector<char> &vec, const std::string &s);
     static void append(std::vector<char> &vec, const char *s);
     static void append(std::vector<char> &vec, char c);
    template<size_t size>
    static inline void append(std::vector<char> &vec, const char (&s)[size]);

    template<typename Handler>
    inline void post(const Handler &handler);

    boost::asio::strand strand;
    boost::asio::generic::stream_protocol::socket socket;
    RedisParser redisParser;
    boost::array<char, 4096> buf;
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
            const boost::asio::ip::address &address,
            unsigned short port,
            std::function<void(bool, const std::string &)> handler);

    // Connect to redis server
     void connect(
            const boost::asio::ip::tcp::endpoint &endpoint,
            std::function<void(bool, const std::string &)> handler);


     void connect(
            const boost::asio::local::stream_protocol::endpoint &endpoint,
            std::function<void(bool, const std::string &)> handler);


    // backward compatibility
    inline void asyncConnect(
            const boost::asio::ip::address &address,
            unsigned short port,
            std::function<void(bool, const std::string &)> handler)
    {
        connect(address, port, std::move(handler));
    }

    // backward compatibility
    inline void asyncConnect(
            const boost::asio::ip::tcp::endpoint &endpoint,
            std::function<void(bool, const std::string &)> handler)
    {
        connect(endpoint, handler);
    }

    // Return true if is connected to redis.
    // Deprecated: use state() == RedisAsyncClisend::State::Connected.
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

class RedisSyncClient : boost::noncopyable {
public:
    typedef RedisClientImpl::State State;

     RedisSyncClient(boost::asio::io_service &ioService);
     ~RedisSyncClient();

    // Connect to redis server
     bool connect(
            const boost::asio::ip::tcp::endpoint &endpoint,
            std::string &errmsg);

    // Connect to redis server
     bool connect(
            const boost::asio::ip::address &address,
            unsigned short port,
            std::string &errmsg);


     bool connect(
            const boost::asio::local::stream_protocol::endpoint &endpoint,
            std::string &errmsg);


    // Set custom error handler.
     void installErrorHandler(
        std::function<void(const std::string &)> handler);

    // Execute command on Redis server with the list of arguments.
     RedisValue command(
            const std::string &cmd, std::deque<RedisBuffer> args);

    // Return connection state. See RedisClientImpl::State.
     State state() const;

protected:
     bool stateValid() const;

private:
    std::shared_ptr<RedisClientImpl> pimpl;
};

}






