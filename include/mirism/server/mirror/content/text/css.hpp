# pragma once
# include <mirism/server/mirror/content/text/base.hpp>

namespace mirism::server::mirror::content::text
{
	class Css
		:	virtual public Base,
			public Logger::ObjectMonitor<Css>
	{
		public:
			using Base::patch;

			// 替换 url(xxxxx) 和 @import "xxxx"
			static
				std::string
				patch
				(std::string, const Request&, const DomainStrategy&);
			virtual
				std::string
				patch_virtual
				(std::string, const Request&, const DomainStrategy&) const
				override;

			const type_set_t&
				get_type_set
				() const
				override;
	};
}