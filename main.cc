#include <iostream>
#include <thread>
#include <cstdlib>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace ba = ::boost::asio;
namespace bb = ::boost::beast;

int main(int argc, char**argv)
{
	int const n_threads = 4;
	ba::io_context io{n_threads};
	::std::vector<::std::thread> thread_pool{};
	thread_pool.reserve(n_threads - 1);
	for (auto i = n_threads - 1; i > 0; --i) {
		thread_pool.emplace_back([&io] { io.run(); });
	}
	io.run();
	return EXIT_SUCCESS;
}
