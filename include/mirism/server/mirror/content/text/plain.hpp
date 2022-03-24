# pragma once
# include <mirism/server/mirror/content/text/base.hpp>

namespace mirism::server::mirror::content::text
{
	class Plain
		:	public Base,
			public Logger::ObjectMonitor<Plain>
	{
		public:
			const type_set_t&
				get_type_set
				() const
				override;
	};
}
