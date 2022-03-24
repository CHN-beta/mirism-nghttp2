# pragma once
# include <mirism/server/common.hpp>

namespace mirism::server
{
	inline
		bool
		valid_method
		(std::string method)
	{
		return std::set<std::string>{"GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE", "PATCH"}
			.contains(method);
	}
	inline
		bool
		method_have_request_body
		(std::string method)
	{
		return std::set<std::string>{"POST", "PUT", "DELETE", "PATCH"}.contains(method);
	}
	inline
		bool
		method_have_response_body
		(std::string method)
	{
		return std::set<std::string>{"GET", "POST", "DELETE", "CONNECT", "OPTIONS", "PATCH"}.contains(method);
	}
}
