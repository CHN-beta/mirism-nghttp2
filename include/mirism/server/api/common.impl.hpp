# pragma once
# include <mirism/server/api/common.hpp>

namespace mirism::server::api
{
	inline
		Request::Request
		(
			boost::asio::ip::tcp::endpoint
				remote,
			std::string
				mirism_host,
			std::string
				method,
			std::string
				path,
			std::multimap<std::string, std::string>
				parameter,
			nghttp2::asio_http2::header_map
				header,
			std::string
				content
		)
		:	Remote(std::move(remote)),
			MirismHost(std::move(mirism_host)),
			Method(std::move(method)),
			Path(std::move(path)),
			Parameter(std::move(parameter)),
			Header(std::move(header)),
			Content(std::move(content))
		{}

	inline
		Response::Response
		(
			unsigned
				status,
			nghttp2::asio_http2::header_map
				header,
			std::string
				content
		)
		:	Status(status),
			Header(std::move(header)),
			Content(std::move(content))
	{}
}
