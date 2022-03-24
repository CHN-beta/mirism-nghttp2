# pragma once
# include <mirism/server/mirror/content/text/base.hpp>

namespace mirism::server::mirror::content::text
{
	class Json
		:	public Base,
			public Logger::ObjectMonitor<Json>
	{
		public:
			const type_set_t&
			get_type_set
			() const
			override;
	};
}
