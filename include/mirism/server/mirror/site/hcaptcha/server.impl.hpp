# pragma once
# include <mirism/server/mirror/site/hcaptcha/server.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::mirror::site::hcaptcha
{
	inline
	const DomainStrategy& Server::get_domain_strategy() const
	{
		static DomainStrategy domain_strategy
		(
			{
				{
					"hcaptcha",
					"hcaptcha.com",
					{
						"hcaptcha.com",
						"newassets.hcaptcha.com",
						"imgs.hcaptcha.com",
						"accounts.hcaptcha.com"
					}
				},
				{
					"ddos-guard",
					"ddos-guard.net",
					{
						"check.ddos-guard.net"
					}
				}
			},
			{R"(.*\.hcaptcha\.com)"_re},
			{}
		);
		return domain_strategy;
	}
}
