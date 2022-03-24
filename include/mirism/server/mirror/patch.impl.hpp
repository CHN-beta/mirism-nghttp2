# pragma once
# include <mirism/server/mirror/patch.hpp>
# include <mirism/server/mirror/content/text/base.hpp>

namespace mirism::server::mirror::patch
{
	namespace detail_
	{
		inline
		PatchMapProxy::operator std::unordered_multimap<PatchTiming, patch_t>() const&
		{
			return PatchMap;
		}
		inline
		PatchMapProxy::operator std::unordered_multimap<PatchTiming, patch_t>() &&
		{
			return std::move(PatchMap);
		}
		inline
		PatchMapProxy PatchMapProxy::operator+(const PatchMapProxy& r) const&
		{
			auto rtn = *this;
			rtn.PatchMap.insert(r.PatchMap.begin(), r.PatchMap.end());
			return rtn;
		}
		inline
		PatchMapProxy PatchMapProxy::operator+(PatchMapProxy&& r) &&
		{
			PatchMap.merge(r.PatchMap);
			return std::move(*this);
		}
	}

	inline
	detail_::PatchMapProxy origin_restrict
	(std::unordered_set<std::string> host_set, std::unordered_set<std::string> ip_set, std::string redirect_url)
	{
		return
		{{{
			PatchTiming::BeforeAllPatch,
			[host_set = std::move(host_set), ip_set = std::move(ip_set), redirect_url = std::move(redirect_url)]
			(
				const std::unique_ptr<Request>& request, std::unique_ptr<Response>& response,
				const DomainStrategy&
			) -> bool
			{
				auto redirect = [&response, &redirect_url] -> bool
				{
					Logger::Guard log;
					response = std::make_unique<Response>();
					response->Status = 302;
					response->Header.insert({"location", {redirect_url, false}});
					log.info("redirect to index");
					return false;
				};
				auto get_host = [&request] -> cppcoro::generator<std::optional<std::string>>
				{
					auto range = request->Header.equal_range("referer");
					for (auto it = range.first; it != range.second; it++)
					{
						if (std::smatch match; std::regex_match(it->second.value, match, "https://([^/]+)/.*"_re))
							co_yield match[1].str();
						else
							co_yield std::nullopt;
					}
				};
				if (request && !ip_set.contains(request->Remote.address().to_string()))
					for (const auto& host : get_host())
						if (!host || (*host != request->MirismHost && !host_set.contains(*host)))
							return redirect();
				return true;
			}
		}}};
	}

	inline
	detail_::PatchMapProxy check_leakage(const std::string& keyword)
	{
		return
		{{
			{
				PatchTiming::AfterRequestHeaderPatch,
				[keyword]
				(
					const std::unique_ptr<Request>& request, std::unique_ptr<Response>&, const DomainStrategy&
				) -> bool
				{
					Logger::Guard log;
					if (request)
						for (const auto& header : request->Header)
							if (header.first.contains(keyword) || header.second.value.contains(keyword))
							{
								auto msg = "found header leakage in {} {} \"{}: {}\""_f
								(
									request->Host, request->Path, header.first, header.second
								);
								Logger::notify(msg);
								log.info(msg);
							}
					return true;
				}
			},
			{
				PatchTiming::BeforeRequestBodyWrite,
				[keyword]
				(
					const std::unique_ptr<Request>& request, std::unique_ptr<Response>&, const DomainStrategy&
				) -> bool
				{
					Logger::Guard log;
					if (request && request->Content)
						if (auto content = dynamic_cast<content::text::Base*>(request->HandledContent.get()))
							content->patch([&log, &request, &keyword](const std::string& content)
							{
								if (content.contains(keyword))
								{
									auto msg = "found content leakage in {} {} \"{}\""_f
									(
										request->Host, request->Path, content
									);
									Logger::notify(msg);
									log.info(msg);
								}
							});
					return true;
				}
			}
		}};
	}
}
