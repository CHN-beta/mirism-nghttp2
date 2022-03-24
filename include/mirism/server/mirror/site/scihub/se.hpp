# pragma once
# include <mirism/server/mirror/site/scihub/base.hpp>

namespace mirism::server::mirror::site::scihub
{
	class Se : public Base
	{
	public:
		const DomainStrategy& get_domain_strategy() const override;
	};
}
