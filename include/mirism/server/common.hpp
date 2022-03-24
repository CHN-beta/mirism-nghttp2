# pragma once
# include <mirism/common.hpp>

namespace mirism::server
{
	bool
		valid_method
		(std::string);
	bool
		method_have_request_body
		(std::string);
	bool
		method_have_response_body
		(std::string);
}
