# pragma once
# include <mirism/server/base.hpp>
# include <mirism/server/utils/pipe.hpp>

namespace mirism::server
{
	namespace detail_
	{
		class SynchronizedBase
		{
			public:
				struct ServerRequest
					: Logger::ObjectMonitor<ServerRequest>
				{
					boost::asio::ip::tcp::endpoint
						Remote;
					std::string
						MirismHost;
					std::string
						Method;
					std::string
						Path;

					// 当写入此字段时，必须保证 key 全部为小写；当读取此字段时，总可以假定 key 是小写
					nghttp2::asio_http2::header_map
						Header;
					std::shared_ptr<utils::Pipe>
						Content;

					ServerRequest
						()
						= default;
					ServerRequest
						(const ServerRequest&)
						= default;
					ServerRequest
						(ServerRequest&&)
						= default;
					ServerRequest
						(
							boost::asio::ip::tcp::endpoint,
							std::string,
							std::string,
							std::string,
							nghttp2::asio_http2::header_map,
							std::shared_ptr<utils::Pipe>
						);
				};
				struct ServerResponse
					: Logger::ObjectMonitor<ServerResponse>
				{
					unsigned
						Status;

					// 当写入此字段时，必须保证 key 全部为小写；当读取此字段时，总可以假定 key 是小写
					nghttp2::asio_http2::header_map
						Header;
					std::shared_ptr<utils::Pipe>
						Content;

					ServerResponse
						()
						= default;
					ServerResponse
						(const ServerResponse&)
						= default;
					ServerResponse
						(ServerResponse&&)
						= default;
					ServerResponse
						(unsigned, nghttp2::asio_http2::header_map, std::shared_ptr<utils::Pipe>);
				};

				// 将该类的一个指针或引用传递给程序的其它部分，
				// 它们就可以设置连接被关闭时（指服务端 response 的 on_close 回调函数被触发时）调用一些回调函数
				// 此类的指针或者引用将仅仅出现在下面几种场合：
				//	  * 在 public 的 operator() 中，保有一个 shared_ptr，函数结束后销毁
				//	  * 在 protected 的 operator() 中，保有一个引用，用于下层对象设置回调函数，函数结束后销毁
				//		  之所以使用引用而不是 shared_ptr，是为了防止对象所有权的循环
				//	  * 在 nghttp2 的 server request 的回调函数中保有一个 shared_ptr（在 shutdown_callback 的返回值中），
				//		用来触发 shutdown
				class ShutdownCallbackHandler
					:   public std::enable_shared_from_this<ShutdownCallbackHandler>,
						public Logger::ObjectMonitor<ShutdownCallbackHandler>
				{
					public:

						// 尝试设置一个回调函数，若还没有 shutdown，则设置成功且返回 true，否则不设置并返回 false
						bool
							add_callback
							(std::function<void(std::uint32_t)>);

						// 将此函数的返回值交给 nghttp2 的对象，在请求关闭的时候，就会同步地调用一遍所有的回调函数，
						// 调用顺序与设置的顺序相反
						nghttp2::asio_http2::close_cb
							shutdown_callback
							();

						// 检查是否已经结束
						std::optional<std::uint32_t>
							finished
							() const;
					protected:
						mutable std::mutex
							Mutex_;
						std::optional<std::uint32_t>
							Finished_;
						std::vector<std::function<void(std::uint32_t)>>
							Callbacks_;
				};
		};
		std::ostream&
			operator<<
			(std::ostream&, const SynchronizedBase::ServerRequest&);
		std::ostream&
			operator<<
			(std::ostream&, const SynchronizedBase::ServerResponse&);
		template <is_static_string<char> Command>
			class Synchronized
			:	public Base<Command>,
				public SynchronizedBase,
				public Logger::ObjectMonitor<Synchronized<Command>>
		{
			public:
				void
					operator()
					(const nghttp2::asio_http2::server::request&, const nghttp2::asio_http2::server::response&)
					override;

			protected:

				// 处理一个请求，返回响应内容，请求体和响应体通过 pipe 异步传输，可能传入 nullptr，也可能返回 nullptr
				// 此函数不应该在返回后还保留有 ShutdownCallbackHandler 的指针
				virtual
					std::unique_ptr<ServerResponse>
					operator()
					(std::unique_ptr<ServerRequest>, ShutdownCallbackHandler&)
					= 0;
		};
	}
	template <auto command = ""_ss>
		using Synchronized = detail_::Synchronized<std::remove_cvref_t<decltype(command)>>;
}
