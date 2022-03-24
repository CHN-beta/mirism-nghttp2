# pragma once
# include <mirism/server/mirror/content/text/javascript.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::mirror::content::text
{
	inline
		std::string
		Javascript::patch
		(std::string content, const Request& request, const DomainStrategy& domain_strategy)
	{
		Logger::Guard log;
		// "https://xxxx"
		return utils::string::replace
		(
			content, R"r((['"])(https\:\/\/[^'"]+)\1)r"_re,
			[&](const std::smatch& match)
			{
				return "{}{}{}"_f
				(
					match[1].str(),
					url_patch(request, domain_strategy, match[2].str()),
					match[1].str()
				);
			}
		);
	}
	inline
		std::string
		Javascript::patch_virtual
		(std::string content, const Request& request, const DomainStrategy& domain_strategy) const
		{return patch(std::move(content), request, domain_strategy);}

	inline
		const Base::type_set_t&
		Javascript::get_type_set
		() const
	{
		static type_set_t type_set
		{
			"application/javascript",
			"text/javascript"
		};
		return type_set;
	};
}
