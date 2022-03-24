# pragma once
# include <mirism/server/mirror/content/text/json.hpp>

namespace mirism::server::mirror::content::text
{
	inline
		const Base::type_set_t&
		Json::get_type_set
		() const
	{
		static type_set_t type_set
		{
			"application/json"
		};
		return type_set;
	};
}
