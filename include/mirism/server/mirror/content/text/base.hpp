# pragma once
# include <mirism/server/mirror/content/base.hpp>

namespace mirism::server::mirror::content::text
{
	class Base
		:	public content::Base,
			public Logger::ObjectMonitor<Base>
	{
		public:

			// 尝试读取 pipe 中的所有内容并按照指定的 encoding 解压。
			// 如果 encoding 为空或者不支持，则不解压。
			// 如果 pipe 为空或读取失败，则写入空字符串
			Base&
				write
				(const encoding_t&, const std::shared_ptr<utils::Pipe>&)
				override;

			// 将内容压缩后异步写入一个 pipe 并返回这个 pipe
			std::shared_ptr<utils::Pipe>
				read
				()
				override;

			// 使用合适的方式修改内容
			// 对于 Text，不会修改内容
			static
				std::string
				patch
				(std::string, const Request&, const DomainStrategy&);
			virtual
				std::string
				patch_virtual
				(std::string, const Request&, const DomainStrategy&) const;

			// 调用 patch_virtual
			Base&
				patch
				(const Request&, const DomainStrategy&)
				override;

			// 调用自定义函数处理内容
			Base&
				patch
				(std::function<void(content_t&)>)
				override;

		protected:
			encoding_t
				Encoding_;
			content_t
				Content_;
	};
}