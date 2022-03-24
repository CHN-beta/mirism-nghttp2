# pragma once
# include <mirism/server/synchronized.hpp>

namespace mirism::server::api
{
	// 服务端的请求
	struct Request
		: Logger::ObjectMonitor<Request>
	{
		boost::asio::ip::tcp::endpoint
			Remote;
		std::string
			MirismHost;
		std::string
			Method;
		std::string
			Path;
		std::multimap<std::string, std::string>
			Parameter; // Query

		// 当写入此字段时，必须保证 key 全部为小写；当读取此字段时，总可以假定 key 是小写
		nghttp2::asio_http2::header_map
			Header;

		std::string
			Content; // Body

		Request
			()
			= default;
		Request
			(const Request&)
			= default;
		Request
			(Request&&)
			= default;

		// 不会检查参数是否合法，而是直接移动构造
		Request
		(
			boost::asio::ip::tcp::endpoint,
			std::string,
			std::string,
			std::string,
			std::multimap<std::string, std::string>,
			nghttp2::asio_http2::header_map,
			std::string
		);
	};
	struct Response
		: Logger::ObjectMonitor<Response>
	{
		unsigned
			Status;

		// 当写入此字段时，必须保证 key 全部为小写；当读取此字段时，总可以假定 key 是小写
		nghttp2::asio_http2::header_map
			Header;

		std::string
			Content; // Body

		Response
			()
			= default;
		Response
			(const Response&)
			= default;
		Response
			(Response&&)
			= default;

		Response
			(unsigned, nghttp2::asio_http2::header_map, std::string);
	};
}
