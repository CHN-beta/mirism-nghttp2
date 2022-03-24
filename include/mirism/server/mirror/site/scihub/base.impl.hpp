# pragma once
# include <mirism/server/mirror/site/scihub/base.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::mirror::site::scihub
{
	inline
	Base::Base()
	{
		Logger::Guard log(fmt::ptr(this));
		PatchMap_.insert
		({
			PatchTiming::BeforeAllPatch,
			[]
			(
				const std::unique_ptr<Request>& request,
				const std::unique_ptr<Response>&,
				const DomainStrategy&
			) -> bool
			{
				Logger::Guard log(request);
				if (!request)
					return true;

				// fix /http(s)%3A/ to /http(s)%3A//
				if
				(
					std::smatch match;
					std::regex_match(request->Path, match, "/(http|https)%3A/(.*)"_re)
				)
				{
					log.debug("fix scihub path {}"_f(request->Path));
					request->Path = "/{}%3A//{}"_f(match[1].str(), match[2].str());
				}

				return true;
			}
		});
	}
}
