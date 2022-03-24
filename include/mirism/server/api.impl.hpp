# pragma once
# include <mirism/server/api.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server
{
	template<std::derived_from<api::Base>... Ts> inline
		Api<Ts...>::Api
		(patch_t)
		:	ApiMap_
			(
				[]
				{
					std::unordered_map<std::string, std::unique_ptr<api::Base>> rtn;
					(rtn.emplace(psmf<&Ts::get_subcommand>()(), std::make_unique<Ts>()), ...);
					return rtn;
				}
				()
			)
		{}
	template<std::derived_from<api::Base>... Ts> inline
		auto
		Api<Ts...>::operator()
		(std::unique_ptr<ServerRequest> request, ShutdownCallbackHandler&)
		-> std::unique_ptr<ServerResponse>
	{
		Logger::Guard log;
		if (!request || !request->Path.starts_with("/api/"))
			return nullptr;
		auto sub_command = request->Path.substr(5);
		if (sub_command.contains('/'))
			sub_command = sub_command.substr(0, sub_command.find('/'));
		if (auto it = ApiMap_.find(sub_command); it != ApiMap_.end())
		{
			auto api_request = std::make_unique<api::Request>();
			api_request->Remote = std::move(request->Remote);
			api_request->MirismHost = std::move(request->MirismHost);
			api_request->Method = std::move(request->Method);
			api_request->Path = std::move(request->Path);

			if (auto pos = api_request->Path.find('?'); pos != std::string::npos)
			{
				for (auto str : utils::string::split(api_request->Path.substr(pos + 1), '&'))
				{
					if (auto pos = str.find('='); pos != std::string::npos)
						api_request->Parameter.emplace(str.substr(0, pos), str.substr(pos + 1));
					else
						api_request->Parameter.emplace(str, "");
				}
				api_request->Path = api_request->Path.substr(0, pos);
			}

			api_request->Header = std::move(request->Header);
			api_request->Content = request->Content ? request->Content->read_all(10s).value_or("") : "";

			auto api_response = (*it->second)(std::move(api_request));
			if (!api_response)
				return nullptr;
			auto response = std::make_unique<ServerResponse>();
			response->Status = api_response->Status;
			response->Header = std::move(api_response->Header);
			response->Content = std::make_shared<utils::Pipe>();
			log.fork
			(
				[content = std::move(api_response->Content), pp = response->Content]
				{
					pp->write(content, 10s);
				}
			);
			return response;
		}
		else
			return nullptr;
	}
}
