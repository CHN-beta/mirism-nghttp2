# pragma once
# include <mirism/server/mirror/site/base.hpp>
# include <mirism/server/mirror/content/text/html.hpp>
# include <mirism/server/mirror/content/text/json.hpp>
# include <mirism/server/mirror/content/text/plain.hpp>
# include <mirism/server/mirror/content/binary.hpp>

namespace mirism::server::mirror::site::google
{
	class Search
		:	public Base
			<
				std::tuple<content::Binary<>, content::text::Plain>,
				std::tuple
				<
					content::Binary<false>,
					content::text::Css,
					content::text::Html,
					content::text::Javascript,
					content::text::Json,
					content::text::Plain
				>
			>
	{
		public:
			Search
				();

			const DomainStrategy&
				get_domain_strategy
				() const
				override;
	};
}
