# pragma once
# include <mirism/server/mirror/site/base.hpp>

namespace mirism::server::mirror::site::hcaptcha
{
	class Client
		: public virtual Base<>
	{
		public:
			Client
				();
	};
}
