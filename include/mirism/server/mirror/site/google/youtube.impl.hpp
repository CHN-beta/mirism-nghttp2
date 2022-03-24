# pragma once
# include <mirism/server/mirror/site/google/youtube.hpp>

namespace mirism::server::mirror::site::google
{
	inline
		const DomainStrategy&
		Youtube::get_domain_strategy
		() const
	{
		static DomainStrategy domain_strategy
		(
			{
				{
					"youtube",
					"youtube.com",
					{
						"accounts.youtube.com"
					}
				}
			},
			{},
			{}
		);
		return domain_strategy;
	}
}
