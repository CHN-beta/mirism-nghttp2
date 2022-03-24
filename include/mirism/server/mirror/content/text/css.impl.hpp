# pragma once
# include <mirism/server/mirror/content/text/css.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::mirror::content::text
{
	inline
		std::string
		Css::patch
		(std::string content, const Request& request, const DomainStrategy& domain_strategy)
	{
		Logger::Guard log;
		// url(xxxxx)   (not starts with data:)
		content = utils::string::replace
		(
			content, R"r(\burl\((?!data:)('|"|)([^'"\),]+)\1\))r"_re,
			[&](const std::smatch& match)
			{
				return "url({0}{1}{0})"_f
				(
					match[1].str(),
					url_patch(request, domain_strategy, match[2].str())
				);
			}
		);

		// @import "xxxx"
		content = utils::string::replace
		(
			content, R"r(@import(\s+)('|"|)([^'"\s,]+)\2)r"_re,
			[&](const std::smatch& match)
			{
				return "@import{0}{1}{2}{1}"_f
				(
					match[1].str(),
					match[2].str(),
					url_patch(request, domain_strategy, match[2].str())
				);
			}
		);
		return content;
	}
	inline
		std::string
		Css::patch_virtual
		(std::string content, const Request& request, const DomainStrategy& domain_strategy) const
		{return patch(std::move(content), request, domain_strategy);}
	inline
		const Base::type_set_t&
		Css::get_type_set
		() const
	{
		static type_set_t type_set
		{
			"text/css"
		};
		return type_set;
	};
}
