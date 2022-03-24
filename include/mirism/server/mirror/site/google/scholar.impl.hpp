# pragma once
# include <mirism/server/mirror/site/google/scholar.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::mirror::site::google
{
	inline
		Scholar::Scholar
		()
	{
		Logger::Guard log(fmt::ptr(this));
		PatchMap_.insert
		(
			{
				PatchTiming::AtResponseBodyPatch,
				[]
				(
					const std::unique_ptr<Request>& request,
					const std::unique_ptr<Response>& response,
					const DomainStrategy& domain_strategy
				) -> bool
				{
					Logger::Guard log(request, response);
					if (!request || !response)
						return true;

					// /scholar? or /scholar_settings? or / or /?
					// html "/scholar_(complete|hpt|url|bfnav|gsb_promo_ping)(?|")
					if
					(
						request->Path.starts_with("/scholar?")
						|| request->Path.starts_with("/scholar_settings?")
						|| request->Path == "/"
						|| request->Path.starts_with("/?")
					)
						if
						(
							auto* content_ptr = dynamic_cast<content::text::Html*>(response->HandledContent.get());
							content_ptr != nullptr
						)
							content_ptr->patch
							(
								[&](content::Base::content_t& content)
								{
									content = utils::string::replace
									(
										content, R"r("/scholar_(complete|hpt|url|bfnav|gsb_promo_ping)(\?|"))r"_re,
										[&](const std::smatch& match)
										{
											return "\"{}{}"_f
											(
												url_patch(*request, domain_strategy, "/scholar_" + match[1].str()),
												match[2].str()
											);
										}
									);
								}
							);

					// replace data-rfr and data-u in search result
					if (request->Path.starts_with("/scholar?"))
						if
						(
							auto* content_ptr = dynamic_cast<content::text::Html*>(response->HandledContent.get());
							content_ptr != nullptr
						)
							content_ptr->patch
							(
								[&](content::Base::content_t& content)
								{
									content = utils::string::replace
									(
										content, R"r((data-rfr|data-u|data-sva|data-tra|data-via)="([^"]+)")r"_re,
										[&](const std::smatch& match)
										{
											return "{}=\"{}\""_f
											(
												match[1].str(),
												url_patch(*request, domain_strategy, match[2].str())
											);
										}
									);
								}
							);

					// /citations? replace '/citatioins? and "\/citations?
					if (request->Path.starts_with("/citations?"))
						if
						(
							auto* content_ptr = dynamic_cast<content::text::Html*>(response->HandledContent.get());
							content_ptr != nullptr
						)
							content_ptr->patch
							(
								[&](content::Base::content_t& content)
								{
									content = utils::string::replace
									(
										content, R"r('(/citations\?))r"_re,
										[&](const std::smatch& match)
										{return "'{}"_f(url_patch(*request, domain_strategy,  match[1].str()));}
									);
									content = utils::string::replace
									(
										content, R"r("\\(/citations\?))r"_re,
										[&](const std::smatch& match)
										{
											auto url = url_patch(*request, domain_strategy,  match[1].str());
											return '"' + utils::string::replace
											(
												url, "/"_re,
												[](const std::smatch&){return "\\/";}
											);
										}
									);
								}
							);

					return true;
				}
			}
		);
	}

	inline
		const DomainStrategy&
		Scholar::get_domain_strategy
		() const
	{
		static DomainStrategy domain_strategy
		(
			{
				{
					"google",
					"google.com",
					{
						"scholar.google.com"
					}
				},
				{
					"default",
					"",
					{
						"scholar.googleusercontent.com"
					}
				}
			},
			{},
			{}
		);
		return domain_strategy;
	}
}
