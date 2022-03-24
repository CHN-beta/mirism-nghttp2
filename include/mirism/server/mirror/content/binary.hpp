# pragma once
# include <mirism/server/mirror/content/base.hpp>

namespace mirism::server::mirror::content
{
	template<bool PlainText = false>
		class Binary
		:	public Base,
			public Logger::ObjectMonitor<Binary<PlainText>>
	{
		public:

			// binary 不允许执行自定义 patch
			Binary<PlainText>&
				patch
				(std::function<void(content_t&)>)
				override;

			const type_set_t&
				get_type_set
				() const
				override;
	};
}
