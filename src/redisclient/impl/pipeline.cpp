/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_PIPELINE_CPP
#define REDISCLIENT_PIPELINE_CPP

#include "redisclient/pipeline.h"
#include "redisclient/redisvalue.h"
#include "redisclient/redissyncclient.h"

namespace redisclient
{

Pipeline::Pipeline(RedisSyncClient &client)
    : client(client)
{
}

Pipeline &Pipeline::command(std::string cmd, std::deque<RedisBuffer> args)
{
    args.push_front(std::move(cmd));
    commands.push_back(std::move(args));
    return *this;
}

RedisValue Pipeline::finish()
{
    return client.pipelined(std::move(commands));
}

RedisValue Pipeline::finish(boost::system::error_code &ec)
{
    return client.pipelined(std::move(commands), ec);
}

}

#endif // REDISCLIENT_PIPELINE_CPP
