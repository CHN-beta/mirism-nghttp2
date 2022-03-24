# pragma once
# include <mirism/server/mirror/common.hpp>

namespace mirism::server::mirror::content
{

	// 临时存储 http body，通过重载虚函数来实现对不同 content-type 的不同存储和处理的方式。
	// 可能使用 pipe 存储，即仅仅存储指针；也可能使用字符串来实际存储内容
	class Base
		: public Logger::ObjectMonitor<Base>
	{
		public:

			// 对应 http 头中的 content-encoding，指示内容的压缩方式，可以为 std::nullopt
			using encoding_t = std::optional<std::string>;

			// http body
			// 并不是所有的派生类都直接使用此字符串来存储内容
			using content_t = std::string;

			// 指示该类对应哪些 content-type
			using type_set_t = std::unordered_set<std::string>;

			virtual
				~Base
				()
				= default;

			// 将 http body 写入此对象
			// 第二个参数可能为 nullptr，这时相当于写入空数据
			// 如果之前已经写入过内容，之前写入的内容将被替换
			// 对于 Base，只是简单地将 pipe 的指针存储起来
			virtual
				Base&
				write
				(const encoding_t&, const std::shared_ptr<utils::Pipe>&);

			// 从此对象中读出 http body
			// 若并没有存储数据，则可能返回 nullptr
			// 读取后，内部将清空
			virtual
				std::shared_ptr<utils::Pipe>
				read
				();

			// 可能调用传入的函数修改内容
			// 对于 Base，忽略传入的函数
			virtual
				Base&
				patch
				(std::function<void(content_t&)>);

			// 使用默认的方式修改内容
			// 对于 Base，不修改内容
			virtual
				Base&
				patch
				(const Request&, const DomainStrategy&);
			virtual
				Base&
				depatch
				(const Request&, const DomainStrategy&);

			// 返回该类对应的 content-type 列表
			// Base 会返回一个空列表
			virtual
				const type_set_t&
				get_type_set
				() const;

		protected:

			// 存储内容
			std::shared_ptr<utils::Pipe>
				Pipe_;
	};
}
