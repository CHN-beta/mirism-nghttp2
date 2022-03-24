# pragma once
# include <mirism/server/api/common.hpp>

namespace mirism::server::api
{
	class Base
		: public Logger::ObjectMonitor<Base>
	{
		public:
			virtual
				~Base()
				= default;

			// 返回 subcommand，路径为 /api/<subcommand> 以及 /api/<subcommand>/xx 的请求会被路由到此类的一个对象
			virtual // static
				const std::string&
				get_subcommand
				() const
				= 0;

			// 处理一个请求
			// 该函数可能会被多线程地调用
			virtual
				std::unique_ptr<Response>
				operator()(std::unique_ptr<Request>)
				= 0;
	};
}
