A header-only redis client library based on redisclient (https://github.com/nekipelov/redisclient/) that was modified to remove boost.asio as a dependency. Simple but powerfull.
Currently the only dependency is asio but there are future plans to remove networking as a dependency. 

Also you can build the library like a shared library. Just use

-DREDIS_CLIENT_DYNLIB and -DREDIS_CLIENT_BUILD to build the library and
-DREDIS_CLIENT_DYNLIB to build your project.


Synchronous get/set example:

```cpp
#include <string>
#include <vector>
#include <iostream>

#include <asio/io_service.hpp>
#include <asio/ip/address.hpp>

#include "redis/sync_client.h"
#include "redis/async_client.h"

int main(int, char**) {
	asio::ip::address address = asio::ip::address::from_string("127.0.0.1");
	const unsigned short port = 6379;

	asio::ip::tcp::endpoint endpoint(address, port);

	asio::io_service io;
	redis::sync_client client(io);

	asio::error_code ec;
	std::string msg = "something";

	client.connect(endpoint, msg);

	if (ec) {
		std::cerr << "Can't connect to redis: " << ec.message() << std::endl;
		return EXIT_FAILURE;
	}

	redis::value result;
	result = client.command("SET", { "key", "value" });

	if (result.isError()) {
		std::cerr << "SET error: " << result.toString() << "\n";
		std::cin.get();
		return EXIT_FAILURE;
	}

	result = client.command("GET", { "key" });

	if (result.isOk()) {
		std::cout << "GET: " << result.toString() << "\n";
		std::cin.get();
		return EXIT_SUCCESS;
	} else {
		std::cerr << "GET error: " << result.toString() << "\n";
		std::cin.get();
		return EXIT_FAILURE;
	}
}
