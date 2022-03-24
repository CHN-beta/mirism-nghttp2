# pragma once
# include <mirism/server/api/compile.hpp>

namespace mirism::server::api
{
	inline
		const std::string&
		Compile::get_subcommand
		() const
	{
		static std::string subcommand = "compile";
		return subcommand;
	}
	inline
		std::unique_ptr<Response>
		Compile::operator()
		(std::unique_ptr<Request> request)
	{
		if (!request || !request->Path.starts_with("/api/compile/"))
			return nullptr;
		static std::map<std::string, std::string> content =
		{
			{
				"time",
				[]
				{
					Logger::Guard log;
					std::stringstream raw_time_str(__DATE__ " " __TIME__ " +08:00");
					std::chrono::sys_time<std::chrono::seconds> time;
					raw_time_str >> date::parse("%b %d %Y %H:%M:%S %z", time);
					if (raw_time_str.fail())
						return "parse failed"s;
					return "{:%m-%d %H:%M:%S}"_f(time);
				}
				()
			}
		};
		if (auto it = content.find(request->Path.substr(13)); it != content.end())
		{
			auto response = std::make_unique<Response>();
			response->Status = 200;
			response->Header.insert({"content-type", {"text/plain", false}});
			response->Header.insert({"access-control-allow-origin", {"*", false}});
			response->Content = it->second;
			return response;
		}
		else
			return nullptr;
	}
}
