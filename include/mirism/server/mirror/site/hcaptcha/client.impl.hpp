# pragma once
# include <mirism/server/mirror/site/hcaptcha/client.hpp>

namespace mirism::server::mirror::site::hcaptcha
{
	inline
		Client::Client
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
			)
			{
				Logger::Guard log(request, response);
				if (!request || !response || !response->HandledContent)
					return true;

				// /.well-known/ddos-guard/* js
				// patch url '/.well-known/ddos-guard/xxx'
				if (request->Path.starts_with("/.well-known/ddos-guard/"))
					if
					(
						auto* content_ptr = dynamic_cast<content::text::Javascript*>(response->HandledContent.get());
						content_ptr != nullptr && dynamic_cast<content::text::Html*>(content_ptr) == nullptr
					)
						content_ptr->patch([&](content::Base::content_t& content)
						{
							content = utils::string::replace
							(
								content,
								R"r('(/\.well\-known/ddos-guard/[^'"]+)')r"_re,
								[&](const std::smatch& match)
									{return R"('{}')"_f(url_patch(*request, domain_strategy, match[1].str()));}
							);
						});
				return true;
			}
		});
	}
}