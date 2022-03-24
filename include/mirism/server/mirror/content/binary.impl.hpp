# pragma once
# include <mirism/server/mirror/content/binary.hpp>

namespace mirism::server::mirror::content
{
	template<bool PlainText> inline
		Binary<PlainText>&
		Binary<PlainText>::patch
		(std::function<void(content_t&)>)
	{
		Logger::Guard log(fmt::ptr(this));
		throw std::runtime_error("Binary is not allow to patch.");
	}
	template<bool PlainText> inline
		const Base::type_set_t&
		Binary<PlainText>::get_type_set
		() const
	{
		static type_set_t type_set = []
		{
			type_set_t type_set
			{
				"application/octet-stream",
				"application/ogg",
				"application/pdf",
				"application/x-endnote-refer",
				"application/x-research-info-systems",
				"application/zip",
				"audio/mpeg",
				"font/otf",
				"font/ttf",
				"font/woff",
				"font/woff2",
				"image/gif",
				"image/jpeg",
				"image/png",
				"image/svg+xml",
				"image/vnd.microsoft.icon",
				"image/webp",
				"image/x-icon"
			};
			if constexpr (PlainText)
				type_set.insert("text/plain");
			return type_set;
		}
		();
		return type_set;
	};
}
