# pragma once
# include <mirism/server/mirror/site/google/patents.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::mirror::site::google
{
	inline
		Patents::Patents
		()
	{
		Logger::Guard log(fmt::ptr(this));
		PatchMap_.insert
		(
			{
				PatchTiming::AfterInternalResponseBodyPatch,
				[]
				(
					const std::unique_ptr<Request>& request,
					const std::unique_ptr<Response>& response,
					const DomainStrategy&
				) -> bool
				{
					Logger::Guard log(request, response);
					if (!request || !response)
						return true;

					// patents.google.com patent/xxxx/xx
					// scan for pdf link and write to content
					if
					(
						request->Host == "patents.google.com"
						&& request->Path.starts_with("/patent/")
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
									std::optional<std::string> url;
									if
									(
										std::smatch match;
										std::regex_search
										(
											content, match,
											R"r(<meta name="citation_pdf_url" content="([^"]+)">)r"_re
										)
									)
									url = match[1].str();
									if (url)
										content = "{}\n{}\n{}"_f
										(
											"<p>Sorry, currently it is not support "
											"to show full google patent page.</p>",
											"<p>But I have found the download link of pdf file of this patent:</p>",
											R"(<a href="{0}">{0}</a>)"_f(*url)
										);
									else
										content = "{}\n{}"_f
										(
											"<p>Sorry, currently it is not support "
											"to show full google patent page.</p>",
											"<p>And I am failed to scan for pdf download link.</p>"
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
		Patents::get_domain_strategy
		() const
	{
		static DomainStrategy domain_strategy
		(
			{
				{
					"google",
					"google.com",
					{
						"patents.google.com",
						"scone-pa.clients6.google.com"
					},
				},
				{
					"googleapis",
					"googleapis.com",
					{
						"patentimages.storage.googleapis.com"
					}
				}
			},
			{},
			{}
		);
		return domain_strategy;
	}
}
