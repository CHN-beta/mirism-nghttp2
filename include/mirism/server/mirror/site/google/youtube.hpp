# pragma once
# include <mirism/server/mirror/site/base.hpp>
# include <mirism/server/mirror/content/binary.hpp>
# include <mirism/server/mirror/content/text/plain.hpp>

namespace mirism::server::mirror::site::google
{
	class Youtube
		:	public Base
			<
				std::tuple<content::Binary<>, content::text::Plain>,
				std::tuple<>
			>
	{
		public:
			const DomainStrategy&
				get_domain_strategy
				() const
				override;
	};
}
