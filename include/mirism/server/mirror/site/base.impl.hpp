# pragma once
# include <mirism/server/mirror/site/base.hpp>
# include <mirism/server/utils/string.hpp>
# include <mirism/server/mirror/content/binary.hpp>

namespace mirism::server::mirror::site
{
	inline
	Base<>::Base()
	{
		Logger::Guard log(fmt::ptr(this));
		PatchMap_.insert(
		{
			{
				PatchTiming::AtRequestHeaderPatch,
				[]
				(
					const std::unique_ptr<Request>& request,
					const std::unique_ptr<Response>& response,
					const DomainStrategy& domain_strategy
				) -> bool
				{
					Logger::Guard log(request, response);
					if (!request)
						return true;

					// patch referer
					auto range = request->Header.equal_range("referer");
					for (auto it = range.first; it != range.second; it++)
						it->second.value = url_depatch(*request, domain_strategy, it->second.value);

					// patch origin
					range = request->Header.equal_range("origin");
					for (auto it = range.first; it != range.second; it++)
						it->second.value = url_depatch(*request, domain_strategy, it->second.value);

					// patch cookie, replace __MHost- with __Host-
					range = request->Header.equal_range("cookie");
					for (auto it = range.first; it != range.second; it++)
						if (it->second.value.starts_with("__MHost-"))
							it->second.value = "__Host-" + it->second.value.substr("__MHost-"s.length());

					return true;
				}
			},
			{
				PatchTiming::AtRequestBodyPatch,
				[]
				(
					const std::unique_ptr<Request>& request,
					const std::unique_ptr<Response>& response,
					const DomainStrategy& domain_strategy
				) -> bool
				{
					Logger::Guard log(request, response);
					if (!request)
						return true;
					if (request->HandledContent)
					{
						log.debug("run request content patch");
						request->HandledContent->depatch(*request, domain_strategy);
					}
					else
						log.debug("request content is null");
					return true;
				}
			},
			{
				PatchTiming::AtResponseHeaderPatch,
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

					// remove content-length
					response->Header.erase("content-length");

					// patch set-cookie
					if (response->Header.contains("set-cookie"))
					{
						auto range = response->Header.equal_range("set-cookie");
						std::vector<std::string> result;
						for (auto it = range.first; it != range.second; it++)
						{
							auto one_result = setcookie_patch(*request, domain_strategy, it->second.value);
							result.insert(result.end(), one_result.begin(), one_result.end());
						}
						response->Header.erase("set-cookie");
						for (const auto& item : result)
							response->Header.insert({"set-cookie", {item, true}});
					}

					// patch report-to
					if (response->Header.contains("report-to"))
					{
						auto range = response->Header.equal_range("report-to");
						for (auto it = range.first; it != range.second; it++)
							it->second.value = utils::string::replace
							(
								it->second.value,
								R"r("url":(\s*)"([^'"]+)")r"_re,
								[&](const std::smatch& match)
								{
									return R"("url":{}"{}")"_f
									(
										match[1].str(),
										url_patch(*request, domain_strategy, match[2].str())
									);
								}
							);
					}

					// patch link
					if (response->Header.contains("link"))
					{
						auto range = response->Header.equal_range("link");
						for (auto it = range.first; it != range.second; it++)
							it->second.value = utils::string::replace
							(
								it->second.value,
								R"r(<(\s*)([^<>]+)(\s*)>)r"_re,
								[&](const std::smatch& match)
								{
									return R"(<{}{}{}>)"_f
									(
										match[1].str(),
										url_patch(*request, domain_strategy, match[2].str()),
										match[3].str()
									);
								}
							);
					}

					// patch content-location
					if (response->Header.contains("content-location"))
					{
						auto range = response->Header.equal_range("content-location");
						for (auto it = range.first; it != range.second; it++)
							it->second.value = url_patch(*request, domain_strategy, it->second.value);
					}

					// patch location
					if (response->Header.contains("location"))
					{
						auto range = response->Header.equal_range("location");
						for (auto it = range.first; it != range.second; it++)
						{
							log.debug("patch location before {}"_f(it->second.value));
							it->second.value = url_patch(*request, domain_strategy, it->second.value);
							log.debug("patch location after {}"_f(it->second.value));
						}
					}

					// discard date
					response->Header.erase("date");

					return true;
				}
			},
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
					if (response->HandledContent)
					{
						log.debug("run response content patch");
						response->HandledContent->patch(*request, domain_strategy);
					}
					else
						log.debug("response content is null");
					return true;
				}
			}
		});
	}

	inline
	std::unique_ptr<Response> Base<>::operator()
	(
		std::unique_ptr<Request> request, const patch_map_t& patch_map,
		const DomainStrategy& domain_strategy, Synchronized<"mirror"_ss>::ShutdownCallbackHandler& shutdown_handler
	)
	{
		Logger::Guard log(request, fmt::ptr(this));
		if (!request)
		{
			log.info("handle nullptr");
			return log.rtn(nullptr);
		}

		log.info("handle {} {}"_f(request->Host, request->Path));
		std::unique_ptr<Response> response;
		auto run_patch = [this, &request, &response, &patch_map, &domain_strategy]
		(const std::vector<PatchTiming>& timing) -> bool
		{
			for (const auto& t : timing)
			{
				auto patch_range = const_cast<const patch_map_t&>(PatchMap_).equal_range(t);
				for (auto it = patch_range.first; it != patch_range.second; it++)
					if (!it->second(request, response, domain_strategy))
						return false;
				patch_range = patch_map.equal_range(t);
				for (auto it = patch_range.first; it != patch_range.second; it++)
					if (!it->second(request, response, domain_strategy))
						return false;
			}
			return true;
		};

		if (shutdown_handler.finished())
			return log.rtn(nullptr);

		if (!run_patch
		({
			PatchTiming::BeforeAllPatch,
			PatchTiming::BeforeRequestHeaderPatch,
			PatchTiming::BeforeInternalRequestHeaderPatch,
			PatchTiming::AtRequestHeaderPatch,
			PatchTiming::AfterInternalRequestHeaderPatch,
			PatchTiming::AfterRequestHeaderPatch,
			PatchTiming::BeforeRequestBodyPatch
		}))
			return response;
		if (!request)
			return log.rtn(nullptr);

		// create request handled_content
		if
		(
			auto type_it = request->Header.find("content-type");
			type_it != request->Header.end() && request->Content && !request->Content->end()
		)
		{
			std::string content_type = type_it->second.value;
			if (std::size_t loc = content_type.find(';'); loc != std::string::npos)
				content_type = content_type.substr(0, loc);
			if (get_request_content().contains(content_type))
			{
				log.debug("use content type {}"_f(content_type));
				request->HandledContent = get_request_content().at(content_type)();
			}
			else
			{
				auto msg = "not supported request content type \"{}\" at {} {}, use default"_f
				(content_type, request->Host, request->Path);
				log.info(msg);
				Logger::notify(msg);
				request->HandledContent = get_default_request_content();
			}
		}

		if (!run_patch({PatchTiming::BeforeRequestBodyRead}))
			return response;
		if (!request)
			return log.rtn(nullptr);

		// write content into handled_content
		if (request->HandledContent && request->Content)
		{
			std::optional<std::string> content_encoding;
			if
			(
				auto encoding_it = request->Header.find("content-encoding");
				encoding_it != request->Header.end()
			)
				content_encoding = encoding_it->second.value;
			request->HandledContent->write(content_encoding, request->Content);
		}

		if (!run_patch
		({
			PatchTiming::AfterRequestBodyRead,
			PatchTiming::BeforeInternalRequestBodyPatch,
			PatchTiming::AtRequestBodyPatch,
			PatchTiming::AfterInternalRequestBodyPatch,
			PatchTiming::BeforeRequestBodyWrite
		}))
			return response;
		if (!request)
			return log.rtn(nullptr);

		// read content from handled_content
		if (request->HandledContent)
			request->Content = request->HandledContent->read();

		if (!run_patch
		({
			PatchTiming::AfterRequestBodyWrite,
			PatchTiming::AfterRequestBodyPatch,
			PatchTiming::BeforeFetchPatch
		}))
			return response;
		if (!request)
			return log.rtn(nullptr);

		// send request and get unpatched header & body
		if (shutdown_handler.finished())
			return log.rtn(nullptr);
		auto request_uniq = std::make_unique<Request>();
		request_uniq->Remote = request->Remote;
		request_uniq->MirismHost = request->MirismHost;
		request_uniq->Method = request->Method;
		request_uniq->Path = request->Path;
		request_uniq->Header = request->Header;
		request_uniq->Content = request->Content;
		request_uniq->Host = request->Host;
		response = fetch(std::move(request_uniq), shutdown_handler);
		if (shutdown_handler.finished() || !response)
			return log.rtn(nullptr);

		if (!run_patch
		({
			PatchTiming::AfterFetchPatch,
			PatchTiming::BeforeResponseHeaderPatch,
			PatchTiming::BeforeInternalResponseHeaderPatch,
			PatchTiming::AtResponseHeaderPatch,
			PatchTiming::AfterInternalResponseHeaderPatch,
			PatchTiming::AfterResponseHeaderPatch,
			PatchTiming::BeforeResponseBodyPatch,
		}))
			return log.rtn(std::move(response));
		if (!response)
			return log.rtn(nullptr);

		// create request handled_content
		if
		(
			auto type_it = response->Header.find("content-type");
			type_it != response->Header.end() && response->Content && !response->Content->end()
		)
		{
			std::string content_type = type_it->second.value;
			if (std::size_t loc = content_type.find(';'); loc != std::string::npos)
				content_type = content_type.substr(0, loc);
			if (get_response_content().contains(content_type))
			{
				log.debug("use content type {}"_f(content_type));
				response->HandledContent = get_response_content().at(content_type)();
			}
			else
			{
				std::string msg;
				if (request)
					msg = "not supported response content type \"{}\" at {} {}, use default"_f
					(content_type, request->Host, request->Path);
				else
					msg = "not supported response content type \"{}\" at unknown request, use default"_f(content_type);
				log.info(msg);
				Logger::notify(msg);
				response->HandledContent = get_default_response_content();
			}
		}

		if (!run_patch({PatchTiming::BeforeResponseBodyRead}))
			return log.rtn(std::move(response));
		if (!response)
			return log.rtn(nullptr);

		// write content into handled_content
		if (response->HandledContent && response->Content)
		{
			std::optional<std::string> content_encoding;
			if
			(
				auto encoding_it = response->Header.find("content-encoding");
				encoding_it != response->Header.end()
			)
				content_encoding = encoding_it->second.value;
			response->HandledContent->write(content_encoding, response->Content);
		}

		if (!run_patch
		({
			PatchTiming::AfterResponseBodyRead,
			PatchTiming::BeforeInternalResponseBodyPatch,
			PatchTiming::AtResponseBodyPatch,
			PatchTiming::AfterInternalResponseBodyPatch,
			PatchTiming::BeforeResponseBodyWrite
		}))
			return log.rtn(std::move(response));
		if (!response)
			return log.rtn(nullptr);

		// read content from handled_content
		if (response->HandledContent)
			response->Content = response->HandledContent->read();

		if (!run_patch
		({
			PatchTiming::AfterResponseBodyWrite,
			PatchTiming::AfterResponseBodyPatch,
			PatchTiming::AfterAllPatch
		}))
			return log.rtn(std::move(response));

		return log.rtn(std::move(response));
	}
	inline
	std::unique_ptr<content::Base> Base<>::get_default_request_content() const
	{
		return std::make_unique<content::Binary<>>();
	}
	inline
	std::unique_ptr<content::Base> Base<>::get_default_response_content() const
	{
		return std::make_unique<content::Binary<>>();
	}

	template<std::derived_from<content::Base>... Ts> inline
	auto Base<>::create_content_map_() -> content_map_t
	{
		if constexpr (sizeof...(Ts) > 0)
		{
			content_map_t result;
			auto add_one = [&result]<std::derived_from<content::Base> T>
			{
				for (const auto& type : psmf<&T::get_type_set>()())
					result[type] = []{return std::make_unique<T>();};
			};
			(add_one.template operator()<Ts>(), ...);
			return result;
		}
		else
			return {};
	};

	template<std::derived_from<content::Base>... Ts> inline
	auto Base<std::tuple<Ts...>, void>::get_request_content() const -> const content_map_t&
	{
		static auto content_map = create_content_map_<Ts...>();
		return content_map;
	}
	template<std::derived_from<content::Base>... Ts> inline
	auto Base<void, std::tuple<Ts...>>::get_response_content() const -> const content_map_t&
	{
		static auto content_map = create_content_map_<Ts...>();
		return content_map;
	}
}
