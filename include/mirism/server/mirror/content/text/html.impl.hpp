# pragma once
# include <mirism/server/mirror/content/text/html.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::mirror::content::text
{
	inline
		std::string
		Html::patch
		(std::string content, const Request& request, const DomainStrategy& domain_strategy)
	{
		Logger::Guard log;
		content = Javascript::patch(std::move(content), request, domain_strategy);
		content = Css::patch(std::move(content), request, domain_strategy);

		// href src action
		content = utils::string::replace
		(
			content, R"r(\b(href|src|action)\s*=\s*('|")([^'"]+)\2)r"_re,
			[&](const std::smatch& match)
			{
				return "{0}={1}{2}{1}"_f
				(
					match[1].str(),
					match[2].str(),
					url_patch(request, domain_strategy, match[3].str())
				);
			}
		);

		// srcset
		content = utils::string::replace
		(
			content, R"r(\b(srcset\s*=\s*)('|")([^'"]+)\2)r"_re,
			[&](const std::smatch& match)
			{
				auto remove_space = [](std::string_view sv)
				{
					auto pos1 = sv.find_first_not_of(' '), pos2 = sv.find_last_not_of(' ');
					if (pos1 <= pos2)
						return sv.substr(pos1, pos2 - pos1 + 1);
					else
						return ""sv;
				};
				std::string content = match[3].str();
				std::string content_after;
				content_after.reserve(content.length() * 2);
				for (auto part_dirty : utils::string::split(content, ','))
				{
					std::string_view part = remove_space(part_dirty);
					if (auto loc = part.find(' '); loc != std::string_view::npos)
						content_after += url_patch(request, domain_strategy, std::string(part.substr(0, loc)))
							+ std::string(part.substr(loc)) + ", ";
					else
						content_after += url_patch(request, domain_strategy, std::string(part)) + ", ";
				}
				return "{}{}{}{}"_f
				(
					match[1].str(),
					match[2].str(),
					content_after.length() >= 2 ? content_after.substr(0, content_after.length() - 2) : content_after,
					match[2].str()
				);
			}
		);
		return content;
	}
	inline
		std::string
		Html::patch_virtual
		(std::string content, const Request& request, const DomainStrategy& domain_strategy) const
		{return patch(std::move(content), request, domain_strategy);}
	inline
		const Base::type_set_t&
		Html::get_type_set
		() const
	{
		static type_set_t type_set
		{
			"text/html"
		};
		return type_set;
	};
}
