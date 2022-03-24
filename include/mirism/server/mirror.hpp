# pragma once
# include <mirism/server/mirror/site/base.hpp>

namespace mirism::server
{
	template<std::derived_from<mirror::site::Base<void, void>>... Ts>
		class Mirror
		:	public Synchronized<"mirror"_ss>,
			public Logger::ObjectMonitor<Mirror<Ts...>>
	{
		public:
			using patch_t = mirror::patch_map_t;
			using host_t = std::string;
			Mirror
				(patch_t);

		protected:
			using mirror_map_t = std::unordered_map<host_t, std::shared_ptr<mirror::site::Base<void, void>>>;
			const patch_t
				Patch_;
			const mirror::DomainStrategy
				DomainStrategy_;
			const mirror_map_t
				MirrorMap_;

			std::unique_ptr<ServerResponse>
				operator()
				(std::unique_ptr<ServerRequest>, ShutdownCallbackHandler&)
				override;
	};
}
