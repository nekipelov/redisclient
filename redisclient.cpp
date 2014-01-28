/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#include <boost/asio/streambuf.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <string>
#include <algorithm>
#include <list>
#include <queue>

#include "redisclient.h"


static void append(std::vector<char> &vec, const std::string &s)
{
    vec.insert(vec.end(), s.begin(), s.end());
}

static void append(std::vector<char> &vec, const char *s)
{
    vec.insert(vec.end(), s, s + strlen(s));
}

template<int size>
static void append(std::vector<char> &vec, const char s[size])
{
    vec.insert(vec.end(), s, s + size);
}

static void append(std::vector<char> &vec, char c)
{
    vec.resize(vec.size() + 1);
    vec[vec.size() - 1] = c;
}


class RedisClientImpl : public boost::enable_shared_from_this<RedisClientImpl> {
public:

    class MatchSize {
    public:
        explicit MatchSize(size_t size) : size(size) {}

        template <typename iterator>
        std::pair<iterator, bool> operator()(iterator begin, iterator end) const
        {
            if( end - begin > static_cast<ptrdiff_t>(size))
                return std::make_pair(begin, true);
            else
                return std::make_pair(begin, false);
        }

    private:
        size_t size;
    };

    RedisClientImpl(boost::asio::io_service &ioService);
    ~RedisClientImpl();

    void handleAsyncConnect(const boost::system::error_code &ec,
                            const boost::function<void(const boost::system::error_code &)> &handler);

    void doCommand(const std::vector<std::string> &command,
                   const boost::function<void(const RedisValue &)> &handler);
    void sendNextCommand();
    void readResponse(const boost::function<void(const RedisValue &)> &handler);
    void processMessage();
    void doProcessMessage(const RedisValue &v);

    void readString(const boost::function<void(const RedisValue &)> &handler);
    void readInt(const boost::function<void(const RedisValue &)> &handler);
    void readBulkItem(const boost::function<void(const RedisValue &)> &handler);
    void readMultiBulkItem(const boost::function<void(const RedisValue &)> &handler);
    void readMultiBulkItemHandler(int recursion, const std::list<RedisValue> &result,
                                  const boost::function<void(const RedisValue &)> &handler);

    void asyncWrite(const boost::system::error_code &ec, const size_t);
    void asyncRead(const boost::function<void(const RedisValue &)> &handler,
                   const boost::system::error_code &ec, const size_t);
    void asyncReadInt(const boost::function<void(const RedisValue &)> &handler,
                      const boost::system::error_code &ec, const size_t);
    void asyncReadString(const boost::function<void(const RedisValue &)> &handler,
                         const boost::system::error_code &ec, const size_t);

    void asyncReadBulkItem(const boost::function<void(const RedisValue &)> &handler,
                           const boost::system::error_code &ec, const size_t);
    void asyncReadBulkItemIntHandler(const boost::function<void(const RedisValue &)> &handler,
                                     const RedisValue &v);
    void readMultiBulkItemIntHandler(const boost::function<void(const RedisValue &)> &handler,
                                     const RedisValue &v);
    void asyncReadMultiBulkItemHandler(int recursion, const std::list<RedisValue> &result,
                                       const boost::function<void(const RedisValue &)> &handler,
                                       const boost::system::error_code &ec, const size_t);
    void recursionStep(int recursion, const std::list<RedisValue> &result,
                       const boost::function<void(const RedisValue &)> &handler,
                       const RedisValue &v);
    void onRedisError(const RedisValue &);
    void defaulErrorHandler(const std::string &s);

    enum {
        NotConnected,
        Connected,
        Subscribed
    } state;

    boost::asio::io_service &ioService;
    boost::asio::ip::tcp::socket socket;
    boost::asio::streambuf inputBuf;
    size_t subscribeSeq;

    typedef std::pair<size_t, boost::function<void(const std::string &s)> > MsgHandlerType;
    typedef boost::function<void(const std::string &s)> SingleShotHandlerType;

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
};

namespace boost {
namespace asio {
  template <> struct is_match_condition<RedisClientImpl::MatchSize>
    : public boost::true_type {};
} // namespace asio
} // namespace boost


/********************** RedisClient **********************************/

RedisClient::RedisClient(boost::asio::io_service &ioService)
{
    pimpl.reset(new RedisClientImpl(ioService));
    
    pimpl->errorHandler = boost::bind(&RedisClientImpl::defaulErrorHandler,
                                      pimpl.get(), _1);
}

RedisClient::~RedisClient()
{
}

bool RedisClient::connect(const char *address, int port)
{
    boost::asio::ip::tcp::resolver resolver(pimpl->ioService);
    boost::asio::ip::tcp::resolver::query query(address,
                                                boost::lexical_cast<std::string>(port));

    boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);

    while(it != boost::asio::ip::tcp::resolver::iterator())
    {
        boost::asio::ip::tcp::endpoint endpoint = *it;
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
            ++it;
        }
    }

    return false;
}

void RedisClient::asyncConnect(const boost::asio::ip::tcp::endpoint &endpoint,
                               const boost::function<void(const boost::system::error_code &)> &handler)
{
    pimpl->socket.async_connect(endpoint,
                                boost::bind(&RedisClientImpl::handleAsyncConnect,
                                            pimpl.get(), _1, handler));
}

void RedisClient::installErrorHandler(
        const boost::function<void(const std::string &)> &handler)
{
    pimpl->errorHandler = handler;
}

void RedisClient::command(const std::string &s, const boost::function<void(const RedisValue &)> &handler)
{
    checkState();

    std::vector<std::string> items(1);
    items[0] = s;

    pimpl->socket.get_io_service().post(
                boost::bind(&RedisClientImpl::doCommand, pimpl.get(), items, handler) );
}

void RedisClient::command(const std::string &cmd, const std::string &arg1,
                          const boost::function<void(const RedisValue &)> &handler)
{
    checkState();

    std::vector<std::string> items(2);
    items[0] = cmd;
    items[1] = arg1;

    pimpl->socket.get_io_service().post(
                boost::bind(&RedisClientImpl::doCommand, pimpl.get(), items, handler) );
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

    pimpl->socket.get_io_service().post(
                boost::bind(&RedisClientImpl::doCommand, pimpl.get(), items, handler) );
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

    pimpl->socket.get_io_service().post(
                boost::bind(&RedisClientImpl::doCommand, pimpl.get(), items, handler) );
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

    pimpl->socket.get_io_service().post(
                boost::bind(&RedisClientImpl::doCommand, pimpl.get(), items, handler) );
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

    pimpl->socket.get_io_service().post(
                boost::bind(&RedisClientImpl::doCommand, pimpl.get(), items, handler) );
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

    pimpl->socket.get_io_service().post(
                boost::bind(&RedisClientImpl::doCommand, pimpl.get(), items, handler) );
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

    pimpl->socket.get_io_service().post(
                boost::bind(&RedisClientImpl::doCommand, pimpl.get(), items, handler) );
}


RedisClient::Handle RedisClient::subscribe(
        const std::string &channel,
        const boost::function<void(const std::string &msg)> &msgHandler,
        const boost::function<void(const RedisValue &)> &handler)
{
    assert( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed);

    static std::string subscribe = "SUBSCRIBE";

    if( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed )
    {
        Handle handle = {pimpl->subscribeSeq++, channel};

        std::vector<std::string> items(2);
        items[0] = subscribe;
        items[1] = channel;

        pimpl->socket.get_io_service().post(
                    boost::bind(&RedisClientImpl::doCommand, pimpl.get(),
                                items, handler) );
        pimpl->msgHandlers.insert(std::make_pair(channel,
                                                 std::make_pair(handle.id, msgHandler)));
        pimpl->state = RedisClientImpl::Subscribed;

        return handle;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << pimpl->state;

        pimpl->errorHandler(ss.str());
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

    static std::string unsubscribe = "UNSUBSCRIBE";

    if( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed )
    {
        // Remove subscribe-handler
        typedef RedisClientImpl::MsgHandlersMap::iterator iterator;
        std::pair<iterator, iterator> pair =
                pimpl->msgHandlers.equal_range(handle.channel);

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
        items[0] = unsubscribe;
        items[1] = handle.channel;

        // Unsubscribe command for Redis
        pimpl->socket.get_io_service().post(
                    boost::bind(&RedisClientImpl::doCommand, pimpl.get(),
                                items, dummyHandler) );
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << pimpl->state;

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

void RedisClient::singleShotSubscribe(const std::string &channel,
                                      const boost::function<void(const std::string &msg)> &msgHandler,
                                      const boost::function<void(const RedisValue &)> &handler)
{
    assert( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed);

    static std::string subscribe = "SUBSCRIBE";

    if( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed )
    {
        std::vector<std::string> items(2);
        items[0] = subscribe;
        items[1] = channel;

        pimpl->socket.get_io_service().post(
                    boost::bind(&RedisClientImpl::doCommand, pimpl.get(),
                                items, handler) );
        pimpl->singleShotMsgHandlers.insert(std::make_pair(channel, msgHandler));
        pimpl->state = RedisClientImpl::Subscribed;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << pimpl->state;

        pimpl->errorHandler(ss.str());
    }
}


void RedisClient::publish(const std::string &channel, const std::string &msg,
                          const boost::function<void(const RedisValue &)> &handler)
{
    assert( pimpl->state == RedisClientImpl::Connected );

    static std::string publish = "PUBLISH";

    if( pimpl->state == RedisClientImpl::Connected )
    {
        std::vector<std::string> items(3);
        items[0] = publish;
        items[1] = channel;
        items[2] = msg;

        pimpl->socket.get_io_service().post(
                    boost::bind(&RedisClientImpl::doCommand, pimpl.get(),
                                items, handler));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << pimpl->state;

        pimpl->errorHandler(ss.str());
    }
}

void RedisClient::checkState() const
{
    assert( pimpl->state == RedisClientImpl::Connected );

    if( pimpl->state != RedisClientImpl::Connected )
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << pimpl->state;

        pimpl->errorHandler(ss.str());
    }
}

RedisClientImpl::RedisClientImpl(boost::asio::io_service &ioService)
    : state(NotConnected), ioService(ioService), socket(ioService), subscribeSeq(0)
{
}

RedisClientImpl::~RedisClientImpl()
{
    boost::system::error_code ignored_ec;
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket.close();
}


void RedisClientImpl::processMessage()
{
    readResponse(boost::bind(&RedisClientImpl::doProcessMessage, shared_from_this(), _1));
}

void RedisClientImpl::doProcessMessage(const RedisValue &v)
{
    if( state == RedisClientImpl::Subscribed )
    {
        std::list<RedisValue> result = v.toList();

        if( result.size() == 3 )
        {
            std::list<RedisValue>::iterator it = result.begin();
            const RedisValue &command = *it++;
            const RedisValue &queue = *it++;
            const RedisValue &RedisValue = *it++;

            const std::string &cmd = command.toString();

            if( cmd == "message" )
            {
                SingleShotHandlersMap::iterator it = singleShotMsgHandlers.find(queue.toString());
                if( it != singleShotMsgHandlers.end() )
                {
                    ioService.post(boost::bind(it->second, RedisValue.toString()));
                    singleShotMsgHandlers.erase(it);
                }
                
                std::pair<MsgHandlersMap::iterator, MsgHandlersMap::iterator> pair =
                        msgHandlers.equal_range(queue.toString());
                for(MsgHandlersMap::iterator it = pair.first; it != pair.second; ++it)
                    ioService.post(boost::bind(it->second.second, RedisValue.toString()));
            }
            else if( cmd == "subscribe" && handlers.empty() == false )
            {
                handlers.front()(v);
                handlers.pop();
            }
            else if(cmd == "unsubscribe" && handlers.empty() == false )
            {
                handlers.front()(v);
                handlers.pop();
            }
            else
            {
                std::stringstream ss;

                ss << "RedisClient::doProcessMessage invalid command: "
                   << command.toString();

                errorHandler(ss.str());
                return;
            }
        }
        else
        {
            errorHandler("Protocol error");
            return;
        }
    }
    else
    {
        if( handlers.empty() == false )
        {
            handlers.front()(v);
            handlers.pop();
        }
        else
        {
            std::stringstream ss;

            ss << "RedisClient::doProcessMessage have a incoming "
                  "message, but handlers is empty.";

            errorHandler(ss.str());
            return;
        }
    }

    socket.get_io_service().post(
                boost::bind(&RedisClientImpl::processMessage, shared_from_this()) );
}

void RedisClientImpl::asyncWrite(const boost::system::error_code &ec, const size_t)
{
    if( ec )
    {
        errorHandler(ec.message());
        return;
    }

    assert( queue.empty() == false );

    const QueueItem &item = queue.front();

    handlers.push( item.handler );
    queue.pop();

    if( queue.empty() == false )
    {
        const QueueItem &item = queue.front();
        boost::asio::async_write(socket,
                                 boost::asio::buffer(item.buff->data(), item.buff->size()),
                                 boost::bind(&RedisClientImpl::asyncWrite,
                                             shared_from_this(), _1, _2));
    }
}

void RedisClientImpl::handleAsyncConnect(const boost::system::error_code &ec,
                                         const boost::function<void(const boost::system::error_code &)> &handler)
{
    if( !ec )
    {
        state = RedisClientImpl::Connected;
        processMessage();
    }

    handler(ec);
}

void RedisClientImpl::doCommand(const std::vector<std::string> &command,
                                const boost::function<void(const RedisValue &)> &handler)
{
    using boost::system::error_code;
    static const char crlf[] = {'\r', '\n'};

    QueueItem item;

    item.buff.reset( new std::vector<char>() );
    item.handler = handler;
    queue.push(item);

    append(*item.buff.get(), '*');
    append(*item.buff.get(), boost::lexical_cast<std::string>(command.size()));
    append(*item.buff.get(), crlf);

    std::vector<std::string>::const_iterator it = command.begin(), end = command.end();
    for(; it != end; ++it)
    {
        append(*item.buff.get(), '$');
        append(*item.buff.get(), boost::lexical_cast<std::string>(it->size()));
        append(*item.buff.get(), crlf);
        append(*item.buff.get(), boost::lexical_cast<std::string>(*it));
        append(*item.buff.get(), crlf);
    }

    if( queue.size() == 1 )
    {
        boost::asio::async_write(socket,
                                 boost::asio::buffer(item.buff->data(), item.buff->size()),
                                 boost::bind(&RedisClientImpl::asyncWrite,
                                             shared_from_this(), _1, _2));
    }
}

void RedisClientImpl::asyncRead(const boost::function<void(const RedisValue &)> &handler,
                                const boost::system::error_code &ec, const size_t)
{
    if( ec )
    {
        errorHandler(ec.message());
        return;
    }

    std::istream stream( &inputBuf );
    char byte = 0;
    stream >> byte;

    switch(byte)
    {
    case '-': // Error
        readString(boost::bind(&RedisClientImpl::onRedisError,
                               shared_from_this(), _1));
        break;
    case '+': // String status
        readString(handler);
        break;
    case ':':
        readInt(handler);
        break;
    case '$':
        readBulkItem(handler);
        break;
    case '*':
        readMultiBulkItem(handler);
        break;
    default:
        errorHandler("Protocol error");
        break;
    }
}

void RedisClientImpl::readResponse(const boost::function<void(const RedisValue &)> &handler)
{
    using boost::system::error_code;
    boost::asio::async_read_until(socket, inputBuf,
                                  RedisClientImpl::MatchSize(1),
                                  boost::bind(&RedisClientImpl::asyncRead,
                                              shared_from_this(), handler, _1, _2));
}

void RedisClientImpl::asyncReadInt(const boost::function<void(const RedisValue &)> &handler,
                                   const boost::system::error_code &ec, const size_t)
{
    if( ec )
    {
        errorHandler(ec.message());
        return;
    }

    std::istream stream( &inputBuf );
    std::string result;

    std::getline(stream, result);

    // remove \r\n
    if( result.empty() == false )
        result.resize( result.size() - 1 );
    handler(atoi(result.c_str()));
}

void RedisClientImpl::readInt(const boost::function<void(const RedisValue &)> &handler)
{
    using boost::system::error_code;
    boost::asio::async_read_until(socket, inputBuf, "\r\n",
                                  boost::bind(&RedisClientImpl::asyncReadInt,
                                              shared_from_this(), handler, _1, _2));
}

void RedisClientImpl::asyncReadString(const boost::function<void(const RedisValue &)> &handler,
                                      const boost::system::error_code &ec, const size_t)
{
    if( ec )
    {
        errorHandler(ec.message());
        return;
    }

    std::istream stream( &inputBuf );
    std::string result;

    std::getline(stream, result);

    // remove \r\n
    if( result.empty() == false )
        result.resize( result.size() - 1 );

    handler(result);
}

void RedisClientImpl::readString(const boost::function<void(const RedisValue &)> &handler)
{
    using boost::system::error_code;
    boost::asio::async_read_until(socket, inputBuf, "\r\n",
                                  boost::bind(&RedisClientImpl::asyncReadString,
                                              shared_from_this(), handler, _1, _2));
}

void RedisClientImpl::asyncReadBulkItem(const boost::function<void(const RedisValue &)> &handler,
                                        const boost::system::error_code &ec, const size_t size)
{
    if( ec )
    {
        errorHandler(ec.message());
        return;
    }

    std::istream stream( &inputBuf );
    std::string result(size, '\0');
    char crlf[2];

    stream.read(&result[0], size);
    stream.read(crlf, sizeof(crlf));

    handler(result);
}

void RedisClientImpl::asyncReadBulkItemIntHandler(
        const boost::function<void(const RedisValue &)> &handler,
        const RedisValue &v)
{
    int size = v.toInt();
    if( size <= 0  )
    {
        handler(RedisValue());
        return;
    }

    boost::asio::async_read_until(socket, inputBuf, RedisClientImpl::MatchSize(size),
                                  boost::bind(&RedisClientImpl::asyncReadBulkItem,
                                              shared_from_this(), handler, _1, size));
}

void RedisClientImpl::readBulkItem(const boost::function<void(const RedisValue &)> &handler)
{
    readInt(boost::bind(&RedisClientImpl::asyncReadBulkItemIntHandler,
                        shared_from_this(), handler, _1));
}

void RedisClientImpl::readMultiBulkItemIntHandler(
        const boost::function<void(const RedisValue &)> &handler,
        const RedisValue &v)
{
    int size = v.toInt();
    if( size <= 0 )
    {
        handler(RedisValue());
    }
    else
    {
        readMultiBulkItemHandler(size, std::list<RedisValue>(), handler);
    }
}

void RedisClientImpl::readMultiBulkItem(const boost::function<void(const RedisValue &)> &handler)
{
    readInt(boost::bind(&RedisClientImpl::readMultiBulkItemIntHandler,
                        shared_from_this(), handler, _1));
}

void RedisClientImpl::recursionStep(int recursion,
                                    const std::list<RedisValue> &result,
                                    const boost::function<void(const RedisValue &)> &handler,
                                    const RedisValue &v)
{
    assert( recursion >= 0 );
    
    std::list<RedisValue> copy(result);
    copy.push_back(v);
    readMultiBulkItemHandler(recursion-1, copy, handler); // next call
}

void RedisClientImpl::asyncReadMultiBulkItemHandler(
        int recursion, const std::list<RedisValue> &result,
        const boost::function<void(const RedisValue &)> &handler,
        const boost::system::error_code &ec, size_t)
{
    if( ec )
    {
        errorHandler(ec.message());
        return;
    }

    boost::function<void(const RedisValue &)> readHandler =
            boost::bind(&RedisClientImpl::recursionStep,
                        shared_from_this(), recursion, result, handler, _1);

    std::istream stream( &inputBuf );
    char byte = 0;
    stream >> byte;

    switch(byte)
    {
    case ':':
        readInt(readHandler);
        break;
    case '$':
        readBulkItem(readHandler);
        break;
    default:
        errorHandler(ec.message());
        break;
    }
}

void RedisClientImpl::readMultiBulkItemHandler(int recursion, const std::list<RedisValue> &result,
                                               const boost::function<void(const RedisValue &)> &handler)
{
    using boost::system::error_code;

    if( recursion == 0 )
    {
        handler(RedisValue(result));
    }
    else
    {
        boost::asio::async_read_until(
                    socket, inputBuf, RedisClientImpl::MatchSize(1),
                    boost::bind(&RedisClientImpl::asyncReadMultiBulkItemHandler,
                                shared_from_this(), recursion, result, handler,
                                _1, _2));
    }
}

void RedisClientImpl::onRedisError(const RedisValue &v)
{
    std::string message = v.toString();
    errorHandler(message);
}


void RedisClientImpl::defaulErrorHandler(const std::string &s)
{
    throw std::runtime_error(s);
}
