# pragma once
# include <mirism/server/mirror/common.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::mirror
{
	inline
	Request::Request(const Request& other, std::unique_ptr<content::Base> handled_content)
	: Synchronized<"mirror"_ss>::ServerRequest(other), Host(other.Host), HandledContent(std::move(handled_content)) {}
	inline
	Request::Request
	(
		boost::asio::ip::tcp::endpoint remote, std::string mirism_host, std::string method,
		std::string path, nghttp2::asio_http2::header_map header, std::shared_ptr<utils::Pipe> content,
		std::string host, std::unique_ptr<content::Base> handled_content
	)
	: Synchronized<"mirror"_ss>::ServerRequest
	(
		std::move(remote), std::move(mirism_host), std::move(method),
		std::move(path), std::move(header), std::move(content)
	), Host(std::move(host)), HandledContent(std::move(handled_content)) {}
	inline
	Response::Response(const Response& other, std::unique_ptr<content::Base> handled_content)
	: Synchronized<"mirror"_ss>::ServerResponse(other), HandledContent(std::move(handled_content)) {}
	inline
	Response::Response
	(
		unsigned status, nghttp2::asio_http2::header_map header, std::shared_ptr<utils::Pipe> content,
		std::unique_ptr<content::Base> handled_content
	)
	: Synchronized<"mirror"_ss>::ServerResponse
	(
		status, std::move(header), std::move(content)
	), HandledContent(std::move(handled_content)) {}

	inline
	std::ostream& operator<<(std::ostream& os, const Request& request)
	{
		os << "MirismHost: {}, Host: {}, Method: {}, Path: {}, Content: {}, HandledContent: {}"_f
		(
			request.MirismHost,
			request.Host,
			request.Method,
			request.Path,
			request.Content,
			request.HandledContent
		);
		return os;
	}
	inline
	std::ostream& operator<<(std::ostream& os, const Response& response)
	{
		os << "Status: {}, Content: {}, HandledContent: {}"_f
		(
			response.Status,
			response.Content,
			response.HandledContent
		);
		return os;
	}

	inline
	DomainStrategy::DomainStrategy
	(
		const std::vector<Group>& list,
		const std::vector<std::regex>& sniff, const std::unordered_set<std::string>& known
	)
	: List(List_), NameMap(NameMap_), BaseMap(BaseMap_), HostMap(HostMap_), Sniff(Sniff_), Known(Known_)
	{
		Logger::Guard log;
		std::unordered_map<std::string, std::unique_ptr<Group>> name_map_temp;
		for (const auto& g : list)
		{
			if (auto it = name_map_temp.find(g.Name); it != name_map_temp.end())
			{
				if (it->second->Base != g.Base)
					log.info("group same name but different base");
				it->second->Host.insert(g.Host.begin(), g.Host.end());
			}
			else
				name_map_temp[g.Name] = std::make_unique<Group>(g);
		}
		for (auto& p : name_map_temp)
			List_.push_back(std::move(p.second));
		build_cache();
		Sniff_ = sniff;
		Known_ = known;
	}

	inline
	DomainStrategy::DomainStrategy(const DomainStrategy& other)
	: List(List_), NameMap(NameMap_), BaseMap(BaseMap_), HostMap(HostMap_), Sniff(Sniff_), Known(Known_),
		Sniff_(other.Sniff_), Known_(other.Known_)
	{
		for (const auto& g : other.List_)
			List_.push_back(std::make_unique<Group>(*g));
		build_cache();
	}

	inline
	DomainStrategy::DomainStrategy(const std::vector<DomainStrategy>& others)
	: List(List_), NameMap(NameMap_), BaseMap(BaseMap_), HostMap(HostMap_), Sniff(Sniff_), Known(Known_)
	{
		Logger::Guard log;
		std::unordered_map<std::string, std::unique_ptr<Group>> name_map_temp;
		for (const auto& o : others)
			for (const auto& g : o.List_)
			{
				if (auto it = name_map_temp.find(g->Name); it != name_map_temp.end())
				{
					if (it->second->Base != g->Base)
						log.info("group same name but different base");
					it->second->Host.insert(g->Host.begin(), g->Host.end());
				}
				else
					name_map_temp[g->Name] = std::make_unique<Group>(*g);
			}
		for (auto& p : name_map_temp)
			List_.push_back(std::move(p.second));
		build_cache();
		for (const auto& o : others)
		{
			Sniff_.insert(Sniff_.end(), o.Sniff_.begin(), o.Sniff_.end());
			Known_.insert(o.Known_.begin(), o.Known_.end());
		}
	}

	inline
	void DomainStrategy::build_cache()
	{
		for (const auto& g : List_)
		{
			NameMap_[g->Name] = std::experimental::observer_ptr<const Group>(g.get());
			BaseMap_[g->Base] = std::experimental::observer_ptr<const Group>(g.get());
			for (const auto& h : g->Host)
				HostMap_[h] = std::experimental::observer_ptr<const Group>(g.get());
		}
	}

	inline
	url_t url_patch(const Request& request, const DomainStrategy& domain_strategy, const url_t& url)
	{
		// 分离出 host 来，并检查是否应该被代理。如果应该，则返回 host，否则返回 std::nullopt
		auto get_host = [&domain_strategy, &request, &url](const std::string& domain_and_path)
		-> std::optional<host_t>
		{
			std::string domain;
			if (auto loc = domain_and_path.find('/'); loc != std::string::npos)
				domain = domain_and_path.substr(0, loc);
			else
				domain = domain_and_path;
			if (domain_strategy.HostMap.contains(domain))
				return domain;
			else
			{
				if (!domain_strategy.Known.contains(domain))
					for (const auto& re : domain_strategy.Sniff)
						if (std::regex_match(domain, re))
						{
							Logger::Guard log;
							auto msg = "sniffed domain {} in {} from {}{}"_f
							(
								domain, url, request.Host, request.Path
							);
							log.info(msg);
							Logger::notify(msg);
							break;
						}
				return std::nullopt;
			}
		};

		if (url.starts_with("https://"))
		{
			if (auto host = get_host(url.substr(8)))
				return "https://{}/mirror/{}/{}"_f
				(
					request.MirismHost, domain_strategy.HostMap.at(*host)->Name, url.substr(8)
				);
			else
				return url;
		}
		else if (url.starts_with("//"))
		{
			if (auto host = get_host(url.substr(2)))
				return "//{}/mirror/{}/{}"_f
				(
					request.MirismHost, domain_strategy.HostMap.at(*host)->Name, url.substr(2)
				);
			else
				return url;
		}
		else if (url.starts_with("/"))
			return "/mirror/{}/{}{}"_f(domain_strategy.HostMap.at(request.Host)->Name, request.Host, url);
		else
			return url;
	}

	inline
	url_t url_depatch(const Request& request, const DomainStrategy& domain_strategy, const url_t& url)
	{
		// 检查 name 和 host 是否对应以及是否应该被代理
		auto check_name_and_host = [&domain_strategy](const std::string& name, const std::string& host) -> bool
		{
			return domain_strategy.HostMap.contains(host) && domain_strategy.HostMap.at(host)->Name == name;
		};

		if (url.starts_with("https://"))
		{
			if
			(
				std::smatch match;
				std::regex_match(url, match, R"(https://([^/]+)/mirror/([^/]+)/([^/]+)(/.*)?)"_re)
					&& match[1].str() == request.MirismHost && check_name_and_host(match[2].str(), match[3].str())
			)
				return "https://{}{}"_f(match[3].str(), match[4].str());
			else
				return url;
		}
		else if (url.starts_with("//"))
		{
			if
			(
				std::smatch match;
				std::regex_match(url, match, R"(//([^/]+)/mirror/([^/]+)/([^/]+)(/.*)?)"_re)
					&& match[1].str() == request.MirismHost && check_name_and_host(match[2].str(), match[3].str())
			)
				return "//{}{}"_f(match[3].str(), match[4].str());
			else
				return url;
		}
		else if (url.starts_with("/"))
		{
			if
			(
				std::smatch match;
				std::regex_match(url, match, R"(/mirror/([^/]+)/([^/]+)(/.*)?)"_re)
					&& check_name_and_host(match[1].str(), match[2].str())
			)
			{
				if (match[3].str() == "")
					return "/";
				else
					return match[3].str();
			}
			else
				return url;
		}
		else
			return url;
	}

	inline
	std::vector<setcookie_t> setcookie_patch
	(const Request& request, const DomainStrategy& domain_strategy, const setcookie_t& setcookie)
	{
		Logger::Guard log;
		log.info("setcookie {}"_f(setcookie));

		// 去除字符串首尾的空格，内部的空格会保留
		auto remove_space = [](std::string_view str) -> std::string
		{
			auto loc1 = str.find_first_not_of(' '), loc2 = str.find_last_not_of(' ');
			if (loc1 == std::string_view::npos)
				return {};
			else if (loc2 >= loc1)
				return std::string(str.substr(loc1, loc2 - loc1 + 1));
			else
				return {};
		};

		// split string with ';'
		auto split_result = utils::string::split(setcookie, ';');
		auto it = split_result.begin();
		if (it == split_result.end())
		{
			log.info("empty setcookie");
			return {};
		}

		// find out name and value
		std::string name, value;
		if (auto loc = it->find('='); loc == std::string_view::npos)
		{
			log.info("failed to find name and value");
			return {};
		}
		else
		{
			name = remove_space(it->substr(0, loc));
			value = remove_space(it->substr(loc + 1));
		}

		// if name is start with __Host-, replace it with __MHost-
		if (name.starts_with("__Host-"))
			name = "__MHost-" + name.substr("__Host-"s.length());

		// find out options
		// lowwer case name, original name, value
		std::map<std::string, std::pair<std::string, std::optional<std::string>>> opts;
		for (it++; it != split_result.end(); it++)
		{
			if (auto loc = it->find('='); loc != std::string_view::npos)
			{
				std::string name = remove_space(it->substr(0, loc)), value = remove_space(it->substr(loc + 1));
				opts.insert({utils::string::lowwer(name), {name, value}});
			}
			else
			{
				std::string name = remove_space(*it);
				opts.insert({utils::string::lowwer(name), {name, std::nullopt}});
			}
		}

		// 根据 domain 和 path 重写 setcookie，可能需要重写为多个
		if (opts.contains("domain") && opts.at("domain").second)
			opts.at("domain").second = utils::string::clean_host(*opts.at("domain").second);
		std::vector<host_t> group_and_domain;
		url_t path;
		if
		(
			(!opts.contains("path") || !opts.at("path").second || *opts.at("path").second == "/")
			&& (opts.contains("domain") && opts.at("domain").second)
			&& domain_strategy.BaseMap.contains(*opts.at("domain").second)
		)   // 找到对应的 group，直接赋值，否则需要逐个域名赋值
		{
			log.info("search success base {}"_f(*opts.at("domain").second));
			group_and_domain.push_back(domain_strategy.BaseMap.at(*opts.at("domain").second)->Name);
			path = "/";
		}
		else
		{
			if (opts.contains("domain") && opts.at("domain").second)
			{
				for (const auto& item : domain_strategy.HostMap)
					if
					(
						item.first == *opts.at("domain").second
						|| item.first.ends_with("." + *opts.at("domain").second)
					)
						group_and_domain.push_back("{}/{}"_f(item.second->Name, item.first));
			}
			else
				group_and_domain.push_back
					("{}/{}"_f(domain_strategy.HostMap.at(request.Host)->Name, request.Host));
			if (opts.contains("path") && opts["path"].second)
				path = *opts["path"].second;
			else
				path = "/";
		}

		// 写入结果
		std::stringstream result_without_path;
		result_without_path << "{}={}; "_f(name, value);
		for (const auto& item : opts)
		{
			if (item.first == "domain" || item.first == "path")
				continue;
			if (item.second.second)
				result_without_path << "{}={}; "_f(item.second.first, *item.second.second);
			else
				result_without_path << "{}; "_f(item.second.first);
		}
		std::vector<setcookie_t> result;
		std::string path_name = opts.contains("path") ? opts.at("path").first : "path";
		for (const auto& item : group_and_domain)
			result.push_back("{}{}=/mirror/{}{}"_f(result_without_path.str(), path_name, item, path));
		return result;
	}

	inline
	std::unique_ptr<Response> fetch
	(std::unique_ptr<Request> request_uniq, Synchronized<"mirror"_ss>::ShutdownCallbackHandler& shutdown_handler)
	{
		std::shared_ptr request(std::move(request_uniq));
		Logger::Guard log(request);

		// check method & content
		if (!valid_method(request->Method))
		{
			log.info("invalid method {}"_f(request->Method));
			return log.rtn(nullptr);
		}
		if
		(
			request->Content
			&& !(method_have_request_body(request->Method) && request->Header.contains("content-type"))
		)
		{
			log.info
			(
				"method {} should not have content, or no content-type header found, "
				"but content {} found, close it now"_f
				(request->Method, request->Content)
			);
			request->Content->shutdown();
			request->Content.reset();
		}

		// create client objs
		struct client_t
		{
			boost::asio::io_service io_service;
			boost::asio::ssl::context tls;
			boost::system::error_code error_code;
			std::unique_ptr<nghttp2::asio_http2::client::session> session;

			client_t() : tls(boost::asio::ssl::context::tls) {}
		};
		auto client = std::make_shared<client_t>();
		client->tls.set_default_verify_paths();
		nghttp2::asio_http2::client::configure_tls_context(client->error_code, client->tls);
		if (client->error_code)
		{
			log.info("client creat error {}"_f(client->error_code));
			return log.rtn(nullptr);
		}

		// if request has been cancelled, return now
		// else, do not destroy client objs until request close
		if (!shutdown_handler.add_callback(Logger::create_callback([client](std::uint32_t) {})))
		{
			log.info("request has been stopped");
			return log.rtn(nullptr);
		}

		// setup client connect (not actually do until IOService_.run())
		log.info("connect to {}"_f(request->Host));
		try
		{
			client->session = std::make_unique<nghttp2::asio_http2::client::session>
				(client->io_service, client->tls, request->Host, "443");
		}
		catch (const std::exception& e)
		{
			log.info("connect error, catch exception: {}"_f(e.what()));
			return log.rtn(nullptr);
		}
		catch (...)
		{
			log.info("connect error");
			return log.rtn(nullptr);
		}

		// following callbacks will actually run in another thread (same thread of io_service.run())
		// they will fill this response object, notify this function "it is safe to return",
		//	  and optionally followed by write response body to pipe
		// It is that the use of response_ptr and shutdown_handler_ptr should be warped in safe_to_return.read,
		//	  to ensure this function have not return
		std::unique_ptr<Response> response;
		std::experimental::observer_ptr<std::unique_ptr<Response>> response_ptr(&response);
		std::experimental::observer_ptr<Synchronized<"mirror"_ss>::ShutdownCallbackHandler>
			shutdown_ptr(&shutdown_handler);
		auto safe_to_return = std::make_shared<utils::Atomic<bool>>(false);

		// session should not have a shared_ptr point to itself
		// thus, we need a weak_ptr to set in callbacks
		std::weak_ptr<client_t> weak_client(client);

		// setup client on_connect callback
		// all ref captures should not be used after set safe_to_return
		client->session->on_connect(Logger::create_callback
		(
			[shutdown_ptr, request, response_ptr, safe_to_return, weak_client]
			(boost::asio::ip::tcp::resolver::iterator)
			{
				Logger::Guard log;
				log.debug("on_connect cb");
				bool return_now = false;

				// if request have been closed, close session now, else close session when request close
				safe_to_return->write([shutdown_ptr, weak_client, &log, &return_now](bool& returned)
				{
					// if returned or closed, close client, or add hook to close client when request is closed
					if (returned || !shutdown_ptr->add_callback
					(
						[weak_client](std::uint32_t)
						{
							if (auto client = weak_client.lock())
								client->io_service.post(Logger::create_callback([weak_client]
								{
									if (auto client = weak_client.lock())
										client->session->shutdown();
								}));
						}
					))
					{
						if (returned)
							log.debug("function has returned");
						else
							log.debug("request has closed");
						if (auto client = weak_client.lock())
							client->session->shutdown();
						returned = true;
						return_now = true;
					}
				});
				if (return_now)
					return;

				auto client = weak_client.lock();
				if (!client)
				{
					log.info("lock failed, client has destructed?");
					return;
				}

				// submit request to upstream
				std::experimental::observer_ptr<const nghttp2::asio_http2::client::request> request_nghttp2;
				if (request->Content)
					request_nghttp2.reset(client->session->submit
					(
						client->error_code,
						request->Method,
						"https://" + request->Host + request->Path,
						request->Content->read(10s),
						request->Header
					));
				else
					request_nghttp2.reset(client->session->submit
					(
						client->error_code,
						request->Method,
						"https://" + request->Host + request->Path,
						request->Header
					));

				// quit if error
				if (client->error_code)
				{
					log.debug("client error {}"_f(client->error_code));
					client->session->shutdown();
					safe_to_return->write([](bool& r){r = true;});
					return;
				}

				// close session when client request close
				request_nghttp2->on_close(Logger::create_callback([weak_client, safe_to_return](uint32_t)
				{
					Logger::Guard log;
					log.debug("on_close cb");
					if (auto client = weak_client.lock())
						client->session->shutdown();
					safe_to_return->write([](bool& r){r = true;});
				}));

				// set on_push cb
				// currently do not support push
				request_nghttp2->on_push(Logger::create_callback
				(
					[host = request->Host, path = request->Path](const nghttp2::asio_http2::client::request&)
					{
						Logger::Guard log;
						log.debug("on_push cb");
						log.info("{} {} received push"_f(host, path));
					}
				));

				// set on_response cb
				request_nghttp2->on_response(Logger::create_callback
				(
					[request, response_ptr, shutdown_ptr, weak_client, safe_to_return]
					(const nghttp2::asio_http2::client::response& response_nghttp2)
					{
						Logger::Guard log;
						log.debug("on_response cb");

						// setup response
						safe_to_return->write([response_ptr, &response_nghttp2, &request, shutdown_ptr](bool& r)
						{
							if (r)
								return;

							auto response = std::make_unique<Response>();
							response->Status = response_nghttp2.status_code();
							for (const auto& [key, value] : response_nghttp2.header())
								response->Header.insert({utils::string::lowwer(key), value});
							if (method_have_response_body(request->Method) && response->Header.contains("content-type"))
							{
								response->Content = std::make_shared<utils::Pipe>();
								response_nghttp2.on_data(Logger::create_callback(response->Content->write(10s)));

								// close pipe when request close
								if (!shutdown_ptr->add_callback
								(
									[weak_pp = std::weak_ptr(response->Content)](std::uint32_t)
									{
										if (auto pp = weak_pp.lock())
											pp->shutdown();
									}
								))
									response->Content->shutdown();
							}

							// bingo! return now
							*response_ptr = std::move(response);
							r = true;
						});
					}
				));
			}
		));

		// connection level error cb
		client->session->on_error(Logger::create_callback
		(
			[host = request->Host, path = request->Path, safe_to_return, weak_client]
			(const boost::system::error_code& code)
			{
				Logger::Guard log;
				log.info("error {} {} {}"_f(host, path, code));
				safe_to_return->write([](bool& r){r = true;});
			}
		));

		// run in another thread
		log.fork([client]{client->io_service.run();});

		// waiting for safe to return
		safe_to_return->wait([](const bool& r){return r;});

		return response;
	}
}
