# pragma once
# include <mirism/server/mirror/content/text/plain.hpp>

namespace mirism::server::mirror::content::text
{
	inline
		const Base::type_set_t&
		Plain::get_type_set
		() const
	{
		static type_set_t type_set
		{
			"application/opensearchdescription+xml",
			"application/reports+json",
			"application/x-www-form-urlencoded",
			"text/ping",
			"text/plain",
		};
		return type_set;
	};
}
