# pragma once
# include <mirism/server.hpp>

namespace mirism
{
	template<std::derived_from<server::Base<>>... Ts> inline
		Server<Ts...>::Server
		(std::unordered_multimap<PatchTiming, patch_t> patch_map, std::tuple<typename Ts::patch_t...> internal_patch)
		:	PatchMap_(std::move(patch_map)),
			ServerMap_
			(
				[&internal_patch]<std::size_t... Index>(std::index_sequence<Index...>)
				{
					return std::unordered_map<std::string_view, std::shared_ptr<server::Base<>>>
					{
						{
							psmf<&Ts::get_command>()(),
							std::make_shared<Ts>(std::get<Index>(std::move(internal_patch)))
						}...
					};
				}(std::make_index_sequence<sizeof...(Ts)>{})
			)
		{}

	template<std::derived_from<server::Base<>>... Ts> inline
		void
		Server<Ts...>::operator()
		(const std::string& mirism_host, const std::string& port)
	{
		Logger::Guard log;
		try
		{
			// create server
			boost::system::error_code ec;
			boost::asio::ssl::context tls(boost::asio::ssl::context::tls);
			tls.use_private_key_file
			(
				"/etc/letsencrypt/live/{}/privkey.pem"_f(mirism_host),
				boost::asio::ssl::context::pem
			);
			tls.use_certificate_chain_file("/etc/letsencrypt/live/{}/fullchain.pem"_f(mirism_host));
			nghttp2::asio_http2::server::configure_tls_context_easy(ec, tls);
			nghttp2::asio_http2::server::http2 nghttp2_server;			
			for (const auto& s : ServerMap_)
			{
				nghttp2_server.handle
				(
					"/{}/"_f(s.first),
					Logger::create_callback
					(
						[server = s.second, this, &mirism_host]
						(
							const nghttp2::asio_http2::server::request& request,
							const nghttp2::asio_http2::server::response& response
						)
						{
							Logger::Guard log;
							try
							{
								auto run_patch = [this, &mirism_host, &server, &request, &response](PatchTiming t)
								{
									auto range = PatchMap_.equal_range(t);
									for (auto it = range.first; it != range.second; ++it)
									{
										if (!it->second(mirism_host, *server, request, response))
											return false;
									}
									return true;
								};
								if (!run_patch(PatchTiming::BeforeHandle))
									return;
								(*server)(request, response);
								if (!run_patch(PatchTiming::AfterHandle))
									return;
							}
							catch (std::exception& ex)
							{
								log.info(R"(catch exception: "{}", try cancel this request and continue)"_f(ex.what()));
								response.cancel();
							}
							catch (...)
							{
								log.info("catch something, try cancel this request and continue");
								response.cancel();
							}
						}
					)
				);
			}
			nghttp2_server.num_threads(64);
			auto msg = "starting server {} on port {}"_f(mirism_host, port);
			Logger::notify(msg);
			log.info(msg);
			if (nghttp2_server.listen_and_serve(ec, tls, "localhost", port))
				log.info("error: {}"_f(ec.message()));
		}
		catch (const std::exception& ex)
		{
			log.info("error: {}"_f(ex.what()));
			throw ex;
		}
	}
}
