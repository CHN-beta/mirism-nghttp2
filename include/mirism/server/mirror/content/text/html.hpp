# pragma once
# include <mirism/server/mirror/content/text/javascript.hpp>
# include <mirism/server/mirror/content/text/css.hpp>

namespace mirism::server::mirror::content::text
{
	class Html
		:	virtual public Base,
			protected Javascript,
			protected Css,
			public Logger::ObjectMonitor<Html>
	{
		public:
			using Base::patch;

			// 先调用 Javascript 和 Css 的 patch，再替换 href src action 和 srcset
			// 以后会改为，仅在需要的地方（例如，style="xxxx"）调用 Javascript 和 Css 的 patch
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
