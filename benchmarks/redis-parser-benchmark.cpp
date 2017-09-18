#include <benchmark/benchmark.h>
#include <redisclient/redisparser.h>


using namespace redisclient;

class ParserFixture : public benchmark::Fixture
{
public:
    ParserFixture()
    {
    }

    void SetUp(const ::benchmark::State &state)
    {
        buffer = generateArray(state.range(0), state.range(1));
        parser = RedisParser();
    }

    void TearDown(const ::benchmark::State &)
    {
        buffer.clear();
    }

    std::string generateArray(size_t size, size_t recursionLevel)
    {
        std::string result;

        result += '*';
        result += std::to_string(size);
        result += "\r\n";

        for(size_t i = 0; i < size; ++i)
        {
            if (recursionLevel > 0)
            {
                result += generateArray(size, recursionLevel - 1);
            }
            else
            {
                result += ':';
                result += std::to_string(i);
                result += "\r\n";
            }
        }

        return result;
    }


    std::string buffer;
    RedisParser parser;
};

BENCHMARK_DEFINE_F(ParserFixture, ArrayParser)(benchmark::State &state)
{
    while (state.KeepRunning())
    {
        state.SetBytesProcessed(buffer.size());
        state.SetItemsProcessed(state.range(0));

        RedisParser::ParseResult result = parser.parse(buffer.c_str(), buffer.size()).second;
        if (result != RedisParser::Completed)
        {
            state.SkipWithError("parse incompleted");
        }
        parser.result();
    }
}

BENCHMARK_DEFINE_F(ParserFixture, ArrayParserByBytes)(benchmark::State &state)
{
    while (state.KeepRunning())
    {
        state.SetBytesProcessed(buffer.size());
        state.SetItemsProcessed(state.range(0));

        for(size_t i = 0; i < buffer.size(); ++i)
        {
            parser.parse(buffer.c_str() + i, 1).second;
        }
        parser.result();
    }
}

template<typename ParserType>
void benchmarkParser(benchmark::State &state, const std::string &buffer, ParserType parser)
{
    while (state.KeepRunning())
    {
        state.SetBytesProcessed(buffer.size());
        state.SetItemsProcessed(1);

        typename ParserType::ParseResult result = parser.parse(buffer.c_str(), buffer.size()).second;
        if (result != ParserType::Completed)
        {
            state.SkipWithError("parse incompleted");
        }
    }
}

BENCHMARK_REGISTER_F(ParserFixture, ArrayParser)
    ->Ranges({{8, 16}, {0, 0}})
    ->Ranges({{8, 16}, {1, 2}});
BENCHMARK_REGISTER_F(ParserFixture, ArrayParserByBytes)
    ->Ranges({{8, 16}, {0, 0}})
    ->Ranges({{8, 16}, {1, 2}});

BENCHMARK_CAPTURE(benchmarkParser, parse_integer, ":123456789\r\n", RedisParser());
BENCHMARK_CAPTURE(benchmarkParser, parse_string, "+simple string\r\n", RedisParser());
BENCHMARK_CAPTURE(benchmarkParser, parse_error, "-error message\r\n", RedisParser());
BENCHMARK_CAPTURE(benchmarkParser, parse_bulk, "$11\r\nbulk string\r\n", RedisParser());
BENCHMARK_CAPTURE(benchmarkParser, parse_array, "*3\r\n:1\r\n:2\r\n:3\r\n", RedisParser());

BENCHMARK_MAIN();
