# pragma once
# include <mirism/server/mirror/site/base.hpp>
# include <mirism/server/mirror/content/binary.hpp>

namespace mirism::server::mirror::site
{
	class Github : public Base<std::tuple<content::Binary<>>, std::tuple<content::Binary<>>>
	{
	public:
		const DomainStrategy& get_domain_strategy() const override;
	};
}
