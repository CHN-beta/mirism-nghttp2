# pragma once
# include <mirism/logger.hpp>
# include <mirism/server/common.hpp>

namespace mirism::server
{
	namespace detail_
	{
		template <is_static_string<char> Command>
			class Base
			:	public std::enable_shared_from_this<Base<Command>>,
				public Logger::ObjectMonitor<Base<Command>>
			{
				public:
					virtual
						~Base
						()
						= default;

					virtual
						void
						operator()
						(const nghttp2::asio_http2::server::request&, const nghttp2::asio_http2::server::response&)
						= 0;

					virtual
						std::string_view
						get_command
						() const
						= 0;
			};
		template <is_static_string<char> Command> requires (Command::Array.size() > 0)
			class Base<Command>
			:	public Base<StaticString<char>>,
				public Logger::ObjectMonitor<Base<Command>>
			{
				public:
					std::string_view
					get_command
					() const
					override;
			};
	}
	template <auto command = ""_ss>
		using Base = detail_::Base<std::remove_cvref_t<decltype(command)>>;
}
