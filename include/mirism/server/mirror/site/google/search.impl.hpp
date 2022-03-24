# pragma once
# include <mirism/server/mirror/site/google/search.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::mirror::site::google
{
	inline
		Search::Search
		()
	{
		Logger::Guard log(fmt::ptr(this));
		PatchMap_.insert
		({
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

				// replace \x22https://adservice.google.com/xxxx\x22 at / and /search?
				if
				(
					request->Host == "www.google.com"
					&& (request->Path == "/" || request->Path.starts_with("/search?"))
				)
					if
					(
						auto* content_ptr = dynamic_cast<content::text::Html*>(response->HandledContent.get());
						content_ptr != nullptr
					)
						content_ptr->patch([&](content::Base::content_t& content)
						{
							content = utils::string::replace
							(
								content, R"r(\\x22(https\:\/\/adservice\.google\.com\/[^\\]+)\\x22)r"_re,
								[&](const std::smatch& match)
									{return "\\x22{}\\x22"_f(url_patch(*request, domain_strategy, match[1].str()));}
							);
						});

				// replace \x22(/advanced_search?|/search?|/search|/preferences?|/url?|/setprefs?|//www.google.com/)
				//	at /search?
				if (request->Host == "www.google.com" && request->Path.starts_with("/search?"))
					if
					(
						auto* content_ptr = dynamic_cast<content::text::Html*>(response->HandledContent.get());
						content_ptr != nullptr
					)
						content_ptr->patch([&](content::Base::content_t& content)
						{
							content = utils::string::replace
							(
								content,
								R"r(\\x22(/advanced_search\?|/search\?|/search|/preferences\?|/url\?)r"
									R"r(|/setprefs\?|//www\.google\.com/))r"_re,
								[&](const std::smatch& match)
									{return "\\x22{}"_f(url_patch(*request, domain_strategy, match[1].str()));}
							);
						});

				// replace "/gen_204" and "gen_204" at / and /search?
				// replace '/xjs/xxx’ at / and /search?
				if
				(
					request->Host == "www.google.com"
					&& (request->Path == "/" || request->Path.starts_with("/search?"))
				)
					if
					(
						auto* content_ptr = dynamic_cast<content::text::Html*>(response->HandledContent.get());
						content_ptr != nullptr
					)
						content_ptr->patch([&](content::Base::content_t& content)
						{
							content = utils::string::replace
							(
								content, R"r("(/)?(gen_204[^"]*)")r"_re,
								[&](const std::smatch& match)
								{
									auto absolute_path = url_patch(*request, domain_strategy, "/" + match[2].str());
									return "\"{}{}\""_f(match[1].str(), absolute_path.substr(1));
								}
							);
							content = utils::string::replace
							(
								content, R"r('(\/xjs\/[^']+)')r"_re,
								[&](const std::smatch& match)
									{return "'{}'"_f(url_patch(*request, domain_strategy, match[1].str()));}
							);
						});

				// replace "/gen_204" "/complete" "/client_204" "/async" at /xjs/_/
				if (request->Host == "www.google.com" && request->Path.starts_with("/xjs/_/"))
					if
					(
						auto* content_ptr = dynamic_cast<content::text::Javascript*>(response->HandledContent.get());
						content_ptr != nullptr
					)
						content_ptr->patch([&](content::Base::content_t& content)
						{
							content = utils::string::replace
							(
								content, R"r("(/(?:gen_204|complete|client_204|async)(?:(?:/|\?)[^"]*)?)")r"_re,
								[&](const std::smatch& match)
									{return "\"{}\""_f(url_patch(*request, domain_strategy, match[1].str()));}
							);
						});

				// replace "/url(?|") at /search? and /xjs/_/
				if
				(
					request->Host == "www.google.com"
					&& (request->Path.starts_with("/search?") || request->Path.starts_with("/xjs/_/"))
				)
					if
					(
						auto* content_ptr = dynamic_cast<content::text::Javascript*>(response->HandledContent.get());
						content_ptr != nullptr
					)
						content_ptr->patch([&](content::Base::content_t& content)
						{
							content = utils::string::replace
							(
								content, R"r("(/url)(\?|"))r"_re,
								[&](const std::smatch& match)
								{
									return "\"{}{}"_f
										(url_patch(*request, domain_strategy, match[1].str()), match[2].str());
								}
							);
						});

				// patch "signin/v2 and "signup/v2
				if (request->Host == "accounts.google.com" && request->Path.starts_with("/Login?"))
					if
					(
						auto* content_ptr = dynamic_cast<content::text::Html*>(response->HandledContent.get());
						content_ptr != nullptr
					)
						content_ptr->patch([&](content::Base::content_t& content)
						{
							content = utils::string::replace
							(
								content, R"r("(signin|signup)\b)r"_re,
								[&](const std::smatch& match)
								{
									auto patched_absolute_path = url_patch
										(*request, domain_strategy, "/" + match[1].str());
									if (patched_absolute_path.length() >= 1)
										return '"' + patched_absolute_path.substr(1);
									else
										return match[0].str();
								}
							);
						});

				// patch www.gstatic.com /patent-search javascript
				// /xhr/xxx
				if (request->Host == "www.gstatic.com" && request->Path.starts_with("/patent-search/"))
					if
					(
						auto* content_ptr = dynamic_cast<content::text::Javascript*>(response->HandledContent.get());
						content_ptr != nullptr
					)
						content_ptr->patch([&](content::Base::content_t& content)
						{
							content = utils::string::replace
							(
								content, R"r("(/xhr/.*)")r"_re,
								[&](const std::smatch& match)
								{
									if
									(
										auto it = domain_strategy.NameMap.find("patents.google.com");
										it != domain_strategy.NameMap.end() && it->second
									)
										return R"r("/mirror/{}/{}{}")r"_f
										(
											it->second->Base,
											"patents.google.com",
											match[1].str()
										);
									else
										return match[0].str();
								}
							);
						});

				return true;
			}
		});

		// currently block some domains
		PatchMap_.insert
		({
			PatchTiming::BeforeAllPatch,
			[](const std::unique_ptr<Request>& request, std::unique_ptr<Response>& response, const DomainStrategy&)
			{
				if (!request)
					return true;
				else if (std::regex_match(request->Host, R"r((ogs|id|adservice|play)\.google\.com)r"_re))
				{
					response = std::make_unique<Response>();
					response->Status = 403;
					return false;
				}
				else
					return true;
			}
		});
	}

	inline
		const DomainStrategy&
		Search::get_domain_strategy
		() const
	{
		static DomainStrategy domain_strategy
		(
			{
				{
					"google",
					"google.com",
					{
						"www.google.com",
						"id.google.com",
						"ogs.google.com",
						"apis.google.com",
						"adservice.google.com",
						"play.google.com",
						"accounts.google.com"
					}
				},
				{
					"gstatic",
					"gstatic.com",
					{
						"www.gstatic.com",
						"fonts.gstatic.com",
						"ssl.gstatic.com",
						"encrypted-tbn0.gstatic.com"
					}
				},
				{
					"googleusercontent",
					"googleusercontent.com",
					{
						"webcache.googleusercontent.com"
					}
				},
				{
					"googleusercontent",
					"googleusercontent.com",
					{
						"lh6.googleusercontent.com"
					}
				},
				{
					"googleapis",
					"googleapis.com",
					{
						"fonts.googleapis.com"
					}
				}
			},
			{},
			{}
		);
		return domain_strategy;
	}
}
