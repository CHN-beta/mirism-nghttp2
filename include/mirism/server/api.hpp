# pragma once
# include <mirism/server/api/base.hpp>

namespace mirism::server
{
	template <std::derived_from<api::Base>... Ts>
		class Api
		:	public Synchronized<"api"_ss>,
			public Logger::ObjectMonitor<Api<Ts...>>
	{
		public:
			struct patch_t
				{};
			Api
				(patch_t);
		protected:
			std::unique_ptr<ServerResponse>
				operator()
				(std::unique_ptr<ServerRequest>, ShutdownCallbackHandler&)
				override;
			std::unordered_map<std::string, std::unique_ptr<api::Base>>
				ApiMap_;
	};
}
