#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/program_options.hpp>

#include <mqtt/async_client.h>

namespace b = ::boost;
namespace b_po = ::boost::program_options;
namespace b_asio = ::boost::asio;
namespace b_beast = ::boost::beast;
namespace b_http = ::boost::beast::http;
using b_tcp = ::boost::asio::ip::tcp;

::std::string g_state{};
::std::shared_mutex g_state_mutex{};

template <typename T>
void HandleRequest(b_http::request<b_http::string_body>&& req, T&& send)
{
	auto const bad_request =
		[&req](::std::string_view reason)
		{
			b_http::response<b_http::string_body> res{
				b_http::status::bad_request, req.version()};
			res.set(b_http::field::server, "1.0");
			res.set(b_http::field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = ::std::string(reason);
			res.prepare_payload();
			return res;
		};

	if (req.method() != b_http::verb::get &&
			req.method() != b_http::verb::head) {
		return send(bad_request("Unknown HTTP-method"));
	}

	if (req.target().empty() ||
			req.target()[0] != '/' ||
			req.target().find("..") != ::std::string_view::npos) {
		return send(bad_request("Illegal request-target"));
	}

	b_http::string_body::value_type body{};
	{
		::std::shared_lock<::std::shared_mutex> lock{g_state_mutex};
		body = g_state;
	}
	auto const size = body.size();

	if (req.method() == b_http::verb::head) {
		b_http::response<b_http::empty_body> res{
			b_http::status::ok, req.version()};
		res.set(b_http::field::server, "1.0");
		res.set(b_http::field::content_type, "application/json");
		res.content_length(size);
		res.keep_alive(req.keep_alive());
		return send(::std::move(res));
	}

	b_http::response<b_http::string_body> res{
		::std::piecewise_construct,
		::std::make_tuple(std::move(body)),
		::std::make_tuple(b_http::status::ok, req.version())};
	res.set(b_http::field::server, "1.0");
	res.set(b_http::field::content_type, "application/json");
	res.content_length(size);
	res.keep_alive(req.keep_alive());
	return send(std::move(res));
}

class Session : public ::std::enable_shared_from_this<Session> {
public:
	Session(b_tcp::socket&& socket):
		stream_(::std::move(socket))
	{
	}

	void Run()
	{
		b_asio::dispatch(
			stream_.get_executor(),
			b_beast::bind_front_handler(
				&Session::DoRead,
				shared_from_this()));
	}

private:
	void DoRead()
	{
		req_ = {};
		stream_.expires_after(::std::chrono::seconds(30));
		b_http::async_read(stream_, buffer_, req_,
			b_beast::bind_front_handler(
				&Session::OnRead,
				shared_from_this()));
	}

	void OnRead(b_beast::error_code ec, ::std::size_t n_bytes)
	{
		b::ignore_unused(n_bytes);
		if (ec == b_http::error::end_of_stream) {
			DoClose();
			return;
		}
		if (ec) {
			::std::cerr << "error: read: " << ec.message() << ::std::endl;
			return;
		}
		auto send_response =
			[this](auto&& msg)
			{
				static_assert(!::std::is_lvalue_reference_v<decltype(msg)>,
						"lvalue is prohibited");
				auto res = ::std::make_shared<
					::std::remove_reference_t<decltype(msg)>>(::std::move(msg));
				res_ = res;
				b_http::async_write(stream_, *res,
					b_beast::bind_front_handler(
						&Session::OnWrite,
						shared_from_this(),
						res->need_eof()));
			};
		HandleRequest(::std::move(req_), send_response);
	}

	void OnWrite(bool close, b_beast::error_code ec, ::std::size_t n_bytes)
	{
		b::ignore_unused(n_bytes);
		if (ec) {
			::std::cerr << "error: write: " << ec.message() << ::std::endl;
			return;
		}
		if (close) {
			DoClose();
			return;
		}
		res_ = nullptr;
		DoRead();
	}

	void DoClose()
	{
		b_beast::error_code ec{};
		stream_.socket().shutdown(b_tcp::socket::shutdown_send, ec);
		if (ec) {
			::std::cerr << "error: close: " << ec.message() << ::std::endl;
			return;
		}
	}

	b_beast::tcp_stream stream_;
	b_beast::flat_buffer buffer_{};
	b_http::request<b_http::string_body> req_{};
	::std::shared_ptr<void> res_{};
};

class Listener : public ::std::enable_shared_from_this<Listener> {
public:
	Listener(b_asio::io_context& io, b_tcp::endpoint endpoint):
		io_(io),
		acceptor_(b_asio::make_strand(io))
	{
		b_beast::error_code ec{};
		acceptor_.open(endpoint.protocol(), ec);
		if (ec) {
			::std::cerr << "error: open: " << ec.message() << ::std::endl;
			return;
		}

		acceptor_.set_option(b_asio::socket_base::reuse_address(true), ec);
		if (ec) {
			::std::cerr << "error: set_option: " << ec.message() << ::std::endl;
			return;
		}

		acceptor_.bind(endpoint, ec);
		if (ec) {
			::std::cerr << "error: bind: " << ec.message() << ::std::endl;
			return;
		}

		acceptor_.listen(b_asio::socket_base::max_listen_connections, ec);
		if (ec) {
			::std::cerr << "error: listen: " << ec.message() << ::std::endl;
			return;
		}
	}

	void Run()
	{
		DoAccept();
	}

private:
	void DoAccept()
	{
		acceptor_.async_accept(
			b_asio::make_strand(io_),
			b_beast::bind_front_handler(
				&Listener::OnAccept,
				shared_from_this()));
	}

	void OnAccept(b_beast::error_code ec, b_tcp::socket socket)
	{
		if (ec) {
			::std::cerr << "error: accept: " << ec.message() << ::std::endl;
			return;
		}
		::std::make_shared<Session>(::std::move(socket))->Run();
		DoAccept();
	}

	b_asio::io_context& io_;
	b_tcp::acceptor acceptor_;
};

class MqttListener : public virtual ::mqtt::callback {
public:
	MqttListener(::mqtt::async_client& client, ::mqtt::connect_options& conn_opts):
		client_(client),
		conn_opts_(conn_opts)
	{
		client_.set_callback(*this);
	}

	void Run()
	{
		DoConnect();
	}

private:
	void DoConnect() {
		try {
			client_.connect(conn_opts_, nullptr, connect_listener_);
		} catch (::mqtt::exception const& e) {
			::std::cerr << "error: mqtt connect: " << e << ::std::endl;
			::std::exit(EXIT_FAILURE);
		}
	}

	void DoReconnect()
	{
		::std::this_thread::sleep_for(::std::chrono::milliseconds(2500));
		try {
			client_.reconnect();
		} catch (::mqtt::exception const& e) {
			::std::cerr << "error: mqtt reconnect:" << e.what() << ::std::endl;
			::std::exit(EXIT_FAILURE);
		}
	}

	void connected(::std::string const& cause) override
	{
		(void) cause;
		int const qos = 1;
		auto const topic = ::std::string{"zigbee2mqtt/sensor0"};
		client_.subscribe(topic, qos, nullptr, subscribe_listener_);
	}

	void message_arrived(::mqtt::const_message_ptr msg) override
	{
		{
			auto state = msg->to_string();
			::std::unique_lock<::std::shared_mutex> lock{g_state_mutex};
			g_state = ::std::move(state);
		}
		::std::cout << "info: mqtt: new message\n";
		::std::cout << "\ttopic: " << msg->get_topic() << "\n";
		::std::cout << "\tpayload: " << msg->to_string() << ::std::endl;
	}

	void delivery_complete(::mqtt::delivery_token_ptr token) override
	{
		(void) token;
	}

	void connection_lost(::std::string const& cause) override
	{
		::std::cerr << "warning: mqtt: connection lost";
		if (!cause.empty()) {
			::std::cerr << ": " << cause;
		}
		::std::cerr << ::std::endl;
		DoReconnect();
	}

	struct ConnectListener: public ::mqtt::iaction_listener {
		MqttListener& p;

		ConnectListener(MqttListener& p):
			p(p)
		{
		}

		void on_success(::mqtt::token const& token) override
		{
			(void) token;
		}

		void on_failure(::mqtt::token const& token) override
		{
			(void) token;
			::std::cerr << "warning: mqtt: connect failed" << ::std::endl;
			p.DoReconnect();
		}
	} connect_listener_{*this};

	struct SubscribeListener : public ::mqtt::iaction_listener {
		MqttListener& p;

		SubscribeListener(MqttListener& p):
			p(p)
		{
		}

		void on_success(::mqtt::token const& token) override
		{
			::std::cerr << "info: mqtt: subscribed" << ::std::endl;
			(void) token;
		}

		void on_failure(::mqtt::token const& token) override
		{
			(void) token;
			::std::cerr << "warning: mqtt: subscribe failed" << ::std::endl;
		}
	} subscribe_listener_{*this};

	::mqtt::async_client& client_;
	::mqtt::connect_options& conn_opts_;
};

int main(int argc, char**argv)
{
	static char const* DEFAULT_MQTT_ADDRESS = "127.0.0.1";
	static char const* DEFAULT_HTTP_ADDRESS = "0.0.0.0";
	static ::std::uint16_t const DEFAULT_MQTT_PORT = 1883;
	static ::std::uint16_t const DEFAULT_HTTP_PORT = 8080;

	b_po::options_description po_desc{"Allowed options"};
	po_desc.add_options()
		("help", "produce help message")
		("mqtt-address",
			b_po::value<::std::string>()->default_value(DEFAULT_MQTT_ADDRESS),
			"mqtt brocker's ip address")
		("mqtt-port",
			b_po::value<::std::uint16_t>()->default_value(DEFAULT_MQTT_PORT),
			"mqtt brocker's port")
		("http-address",
			b_po::value<::std::string>()->default_value(DEFAULT_HTTP_ADDRESS),
			"start listen on this address")
		("http-port",
			b_po::value<::std::uint16_t>()->default_value(DEFAULT_HTTP_PORT),
			"start listen on this port")
		;

	b_po::variables_map po_vm{};
	b_po::store(b_po::parse_command_line(argc, argv, po_desc), po_vm);
	b_po::notify(po_vm);

	if (po_vm.count("help")) {
		::std::cout << po_desc << ::std::endl;
		return EXIT_SUCCESS;
	}

	auto const mqtt_broker_addr =
		static_cast<::std::ostringstream&>(
			::std::ostringstream{}
			<< "tcp://" << po_vm["mqtt-address"].as<::std::string>()
			<< ":"
			<< ::std::to_string(po_vm["mqtt-port"].as<::std::uint16_t>())
		).str();

	auto const mqtt_client_id = ::std::string{"z2m-node-info"};

	::mqtt::async_client mqtt_client{mqtt_broker_addr, mqtt_client_id};
	::mqtt::connect_options mqtt_conn_opts{};
	mqtt_conn_opts.set_clean_session(true);

	MqttListener mqtt_listener(mqtt_client, mqtt_conn_opts);
	mqtt_listener.Run();

	auto const ip_addr = b_asio::ip::make_address("0.0.0.0");
	auto const port = static_cast<unsigned short>(8081);
	int const n_threads = 4;

	b_asio::io_context io{n_threads};

	::std::make_shared<Listener>(io, b_tcp::endpoint{ip_addr, port})->Run();

	::std::vector<::std::thread> thread_pool{};
	thread_pool.reserve(n_threads - 1);
	for (auto i = n_threads - 1; i > 0; --i) {
		thread_pool.emplace_back(
			[&io]
			{
				io.run();
			});
	}
	io.run();

	try {
		mqtt_client.disconnect()->wait();
	} catch (::mqtt::exception const& e) {
		::std::cerr << "error: mqtt disconnect: " << e << ::std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
