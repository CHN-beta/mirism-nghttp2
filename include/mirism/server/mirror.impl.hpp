# pragma once
# include <mirism/server/mirror.hpp>

namespace mirism::server
{
	template<std::derived_from<mirror::site::Base<void, void>>... Ts> inline
		Mirror<Ts...>::Mirror
		(patch_t patch)
		:	Patch_(std::move(patch)),
			DomainStrategy_({psmf<&Ts::get_domain_strategy>()()...}),
			MirrorMap_
			(
				[]
				{
					mirror_map_t mirror_map;
					auto add_mirror_map
						= [&mirror_map]<std::derived_from<mirism::server::mirror::site::Base<void, void>> T>
						{
							auto mirror = std::make_shared<T>();
							for (const auto& g : psmf<&T::get_domain_strategy>()().List)
								for (const auto& h : g->Host)
									mirror_map[h] = mirror;
						};
					(add_mirror_map.template operator()<Ts>(), ...);
					return mirror_map;
				}
				()
			)
		{}
	template<std::derived_from<mirism::server::mirror::site::Base<void, void>>... Ts> inline
		auto
		Mirror<Ts...>::operator()
		(std::unique_ptr<ServerRequest> request_syn, ShutdownCallbackHandler& shutdown_handler)
		-> std::unique_ptr<ServerResponse>
	{
		Logger::Guard log(request_syn, fmt::ptr(this));
		if (std::smatch match; !std::regex_match(request_syn->Path, match, R"r(/mirror/([^/]+)/([^/]+)(/.*)?)r"_re))
			return log.rtn(nullptr);
		else if
		(
			auto it = DomainStrategy_.HostMap.find(match[2].str());
			it == DomainStrategy_.HostMap.end() || it->second->Name != match[1].str()
		)
			return log.rtn(nullptr);
		else
		{
			auto request = std::make_unique<mirror::Request>();
			request->Remote = std::move(request_syn->Remote);
			request->MirismHost = std::move(request_syn->MirismHost);
			request->Method = std::move(request_syn->Method);
			request->Path = (match[3].str().length() == 0) ? std::string("/") : match[3].str();
			request->Header = std::move(request_syn->Header);
			request->Content = std::move(request_syn->Content);
			request->Host = match[2].str();
			auto response = (*MirrorMap_.at(match[2].str()))
				(std::move(request), Patch_,DomainStrategy_, shutdown_handler);
			if (!response)
				return log.rtn(nullptr);
			else
			{
				auto response_syn = std::make_unique<ServerResponse>();
				response_syn->Status = response->Status;
				response_syn->Header = std::move(response->Header);
				response_syn->Content = std::move(response->Content);
				return response_syn;
			}
		}
	}
}
