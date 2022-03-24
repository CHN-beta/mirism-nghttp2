# pragma once
# include <mirism/server/base.hpp>

namespace mirism::server::detail_
{
	template <is_static_string<char> Command> requires (Command::Array.size() > 0) inline
		std::string_view
		Base<Command>::get_command
		() const
		{return Command::StringView;}
}
