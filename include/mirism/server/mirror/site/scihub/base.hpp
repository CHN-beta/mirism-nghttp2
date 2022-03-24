# pragma once
# include <mirism/server/mirror/site/hcaptcha/client.hpp>
# include <mirism/server/mirror/content/text/html.hpp>
# include <mirism/server/mirror/content/text/plain.hpp>
# include <mirism/server/mirror/content/binary.hpp>

namespace mirism::server::mirror::site::scihub
{
	class Base : public virtual hcaptcha::Client, public virtual site::Base
	<
		std::tuple<content::Binary<>, content::text::Plain>,
		std::tuple
		<
			content::Binary<true>,
			content::text::Css,
			content::text::Html,
			content::text::Javascript
		>
	>
	{
	public:
		Base();
	};
}
