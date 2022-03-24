# pragma once
# include <mirism/server/mirror/site/github.hpp>

namespace mirism::server::mirror::site
{
	inline
	const DomainStrategy& Github::get_domain_strategy() const
	{
		static DomainStrategy domain_strategy({{"github", "github.com", {"github.com"}}}, {}, {});
		return domain_strategy;
	}
}
