# pragma once
# include <mirism/server/api/base.hpp>

namespace mirism::server::api
{
	class Compile
		:	public Base,
			public Logger::ObjectMonitor<Compile>
	{
		public:
			const std::string&
				get_subcommand
				() const
				override;

			std::unique_ptr<Response>
				operator()
				(std::unique_ptr<Request>)
				override;
	};
}

