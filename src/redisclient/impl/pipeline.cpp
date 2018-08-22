/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_PIPELINE_CPP
#define REDISCLIENT_PIPELINE_CPP

#include "redisclient/impl/pipeline.h"
#include "redisclient/impl/redisvalue.h"
#include "redisclient/impl/redissyncclient.h"

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

}

#endif // REDISCLIENT_PIPELINE_CPP
