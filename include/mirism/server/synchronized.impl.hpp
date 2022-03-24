# pragma once
# include <mirism/server/synchronized.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::detail_
{
	inline
		SynchronizedBase::ServerRequest::ServerRequest
		(
			boost::asio::ip::tcp::endpoint remote,
			std::string mirism_host,
			std::string method,
			std::string path,
			nghttp2::asio_http2::header_map header,
			std::shared_ptr<utils::Pipe> content
		)
		:	Remote(std::move(remote)),
			MirismHost(std::move(mirism_host)),
			Method(std::move(method)),
			Path(std::move(path)),
			Header(std::move(header)),
			Content(std::move(content))
		{}
	inline
		SynchronizedBase::ServerResponse::ServerResponse
		(unsigned status, nghttp2::asio_http2::header_map header, std::shared_ptr<utils::Pipe> content)
		: Status(status), Header(std::move(header)), Content(std::move(content))
		{}
	inline
		bool
		SynchronizedBase::ShutdownCallbackHandler::add_callback
		(std::function<void(std::uint32_t)> cb)
	{
		std::lock_guard<std::mutex> lock(Mutex_);
		if (!Finished_)
			Callbacks_.push_back(std::move(cb));
		return !Finished_;
	}
	inline
		nghttp2::asio_http2::close_cb
		SynchronizedBase::ShutdownCallbackHandler::shutdown_callback
		()
	{
		return [handler = shared_from_this()](std::uint32_t code)
		{
			Logger::Guard log;
			std::lock_guard<std::mutex> lock(handler->Mutex_);
			log.debug("close on {}, will run {} callbacks"_f(code, handler->Callbacks_.size()));
			handler->Finished_ = code;
			for (auto& cb : handler->Callbacks_ | std::views::reverse)
				cb(code);
		};
	}
	inline
		std::optional<std::uint32_t>
		SynchronizedBase::ShutdownCallbackHandler::finished
		() const
	{
		std::lock_guard<std::mutex> lock(Mutex_);
		return Finished_;
	}

	inline
		std::ostream&
		operator<<
		(std::ostream& os, const SynchronizedBase::ServerRequest& request)
	{
		return os << "MirismHost: {}, Method: {}, Path: {}, Content: {}"_f
		(
			request.MirismHost,
			request.Method,
			request.Path,
			request.Content
		);
	}
	inline
		std::ostream&
		operator<<
		(std::ostream& os, const SynchronizedBase::ServerResponse& response)
	{
		return os << "Status: {}, Content: {}"_f
		(
			response.Status,
			response.Content
		);
	}

	template <is_static_string<char> Command> inline
		void
		Synchronized<Command>::operator
		()
		(
			const nghttp2::asio_http2::server::request& serverreq,
			const nghttp2::asio_http2::server::response& serverres
		)
	{
		struct ServerObjects
			: Logger::ObjectMonitor<ServerObjects>
		{
			std::experimental::observer_ptr<const nghttp2::asio_http2::server::request>
				Request;
			std::experimental::observer_ptr<const nghttp2::asio_http2::server::response>
				Response;
			std::experimental::observer_ptr<boost::asio::io_service>
				IOService;
			bool
				Closed;
		};

		Logger::Guard log(fmt::ptr(&serverreq), fmt::ptr(&serverres), fmt::ptr(this));
		if (!valid_method(serverreq.method()))
		{
			log.info("invalid method {}"_f(serverreq.method()));
			serverres.cancel();
			return;
		}

		// create and setup request
		auto request = std::make_unique<ServerRequest>();
		request->Remote = serverreq.remote_endpoint();
		request->MirismHost = serverreq.uri().host;
		request->Method = serverreq.method();
		request->Path = serverreq.uri().raw_path;
		if (!serverreq.uri().raw_query.empty())
			request->Path.append("?" + serverreq.uri().raw_query);
		for (const auto& [key, value] : serverreq.header())
			request->Header.insert({utils::string::lowwer(key), value});
		if (method_have_request_body(request->Method) && request->Header.contains("content-type"))
			request->Content = std::make_shared<utils::Pipe>();

		log.debug("create request {}"_f(request));

		// create ShutdownCallbackHandler
		auto shutdown_handler = std::make_shared<ShutdownCallbackHandler>();

		// setup callbacks
		serverres.on_close(Logger::create_callback(shutdown_handler->shutdown_callback()));
		if (request->Content)
		{
			serverreq.on_data(Logger::create_callback(request->Content->write(10s)));
			shutdown_handler->add_callback
			(
				[weak_pp = std::weak_ptr(request->Content)](std::uint32_t)
				{
					if (auto pp = weak_pp.lock())
						pp->shutdown();
				}
			);
		}

		// 创建 Atomic<ServerObjects>
		// 在另外的线程中，对 serverreq 和 serverres 的使用必须包裹在该对象的成员函数中，
		// 并且检查是否已经被关闭，如果已经关闭就不能再进行读写
		auto serverobjs = std::make_shared<utils::Atomic<ServerObjects>>();
		serverobjs->write
		(
			[&serverreq, &serverres](ServerObjects& objs)
			{
				objs.Request.reset(&serverreq);
				objs.Response.reset(&serverres);
				objs.IOService.reset(&serverres.io_service());
				objs.Closed = false;
			}
		);

		// 增加回调函数，若关闭时 serverobjs 仍然存在，则将它标记为已关闭
		shutdown_handler->add_callback
		(
			[weak_objs = std::weak_ptr(serverobjs)](std::uint32_t)
			{
				if (auto objs = weak_objs.lock())
					objs->write([](ServerObjects& objs){objs.Closed = true;});
			}
		);

		// 在另一个线程中，同步调用下级对象处理
		log.fork
		(
			[
				request = std::move(request),
				serverobjs,
				sthis = std::static_pointer_cast<Synchronized<Command>>(this->shared_from_this()),
				shutdown_handler
			] mutable
			{
				Logger::Guard log;

				// handle request and write response to serverobj
				log.debug("send request {}"_f(request));
				std::shared_ptr response = (*sthis)(std::move(request), *shutdown_handler);
				log.debug("get response {}"_f(response));
				serverobjs->read
				(
					[&response, &serverobjs](const ServerObjects& sobjs)
					{
						Logger::Guard log;
						if (!sobjs.Closed)
							sobjs.IOService->post
							(
								Logger::create_callback
								(
									[serverobjs, response]
									{
										Logger::Guard log;
										serverobjs->write
										(
											[&response, &log](ServerObjects& sobjs)
											{
												if (!sobjs.Closed)
												{
													if (!response)
													{
														sobjs.Response->write_head(500);
														sobjs.Response->end();
													}
													else
													{
														sobjs.Response->write_head(response->Status, response->Header);
														if
															(
																method_have_response_body(sobjs.Request->method())
																	&& response->Header.contains("content-type")
																	&& response->Content
															)
															sobjs.Response->end(response->Content->read(10s));
														else
															sobjs.Response->end();
													}
												}
												else
													log.info("closed");
											}
										);
									}
								)
							);
						else
							log.info("closed");
					}
				);
			}
		);
	}
}
