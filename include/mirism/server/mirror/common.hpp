# pragma once
# include <mirism/server/synchronized.hpp>

namespace mirism::server::mirror
{
	namespace content
	{
		class Base;
	}

	// 在 mirror 的范围内，存储一个请求相关的信息
	struct Request : Synchronized<"mirror"_ss>::ServerRequest, Logger::ObjectMonitor<Request>
	{
		std::string Host;
		std::unique_ptr<content::Base> HandledContent;

		Request() = default;
		Request(Request&&) = default;
		Request(const Request&, std::unique_ptr<content::Base>);
		Request
		(
			boost::asio::ip::tcp::endpoint, std::string, std::string, std::string,
			nghttp2::asio_http2::header_map, std::shared_ptr<utils::Pipe>,
			std::string, std::unique_ptr<content::Base>
		);
	};
	std::ostream& operator<<(std::ostream&, const Request&);

	// 在 mirror 的范围内，存储一个响应相关的信息
	struct Response : Synchronized<"mirror"_ss>::ServerResponse, Logger::ObjectMonitor<Response>
	{
		std::unique_ptr<content::Base> HandledContent;

		Response() = default;
		Response(Response&&) = default;
		Response(const Response&, std::unique_ptr<content::Base>);
		Response
		(
			unsigned, nghttp2::asio_http2::header_map, std::shared_ptr<utils::Pipe>,
			std::unique_ptr<content::Base>
		);
	};
	std::ostream& operator<<(std::ostream&, const Response&);

	class DomainStrategy
	{
	public:
		struct Group
		{
			std::string Name;
			std::string Base;
			std::unordered_set<std::string> Host;
		};
		DomainStrategy
		(const std::vector<Group>&, const std::vector<std::regex>&, const std::unordered_set<std::string>&);
		DomainStrategy(const DomainStrategy&);
		DomainStrategy(const std::vector<DomainStrategy>&);
		const std::vector<std::unique_ptr<const Group>>& List;
		const std::unordered_map<std::string, std::experimental::observer_ptr<const Group>>
			&NameMap, &BaseMap, &HostMap;
		const std::vector<std::regex>& Sniff;
		const std::unordered_set<std::string>& Known;
	protected:
		std::vector<std::unique_ptr<const Group>> List_;
		std::unordered_map<std::string, std::experimental::observer_ptr<const Group>> NameMap_, BaseMap_, HostMap_;
		std::vector<std::regex> Sniff_;
		std::unordered_set<std::string> Known_;
		void build_cache();
	};

	using url_t = std::string;
	using setcookie_t = std::string;
	using host_t = std::string;

	// 针对具体的网站，常常需要一些除了通用的修改以外的补丁，这些补丁以回调函数的方式存在，在类的构造函数内部或调用类的 operator() 时载入
	// 若一个补丁返回 false，则处理过程立即终止（随后的补丁也不再执行），并向上层返回结果。若返回 true，则会继续执行随后的补丁
	// 不保证传入的指针不是 nullptr，补丁应该对其进行检查，甚至在有必要时将其 reset
	using patch_t = std::function<bool(std::unique_ptr<Request>&, std::unique_ptr<Response>&, const DomainStrategy&)>;

	// 添加补丁时，需要指明执行补丁的时机。被设置为同一个时间的补丁也会被依次执行而不是多线程同时执行，但不保证执行的先后顺序
	// 内置于 content::xxx 和 Base 中的通用修改的执行时机为 AtxxxxxxPatch
	// 若加载的补丁与内置修改的次序无关紧要，则也应该置于 AtxxxxxPatch
	enum class PatchTiming
	{
		BeforeAllPatch,
		BeforeRequestHeaderPatch,
		BeforeInternalRequestHeaderPatch,
		AtRequestHeaderPatch,
		AfterInternalRequestHeaderPatch,
		AfterRequestHeaderPatch,
		BeforeRequestBodyPatch,
		BeforeRequestBodyRead,
		AfterRequestBodyRead,
		BeforeInternalRequestBodyPatch,
		AtRequestBodyPatch,
		AfterInternalRequestBodyPatch,
		BeforeRequestBodyWrite,
		AfterRequestBodyWrite,
		AfterRequestBodyPatch,
		BeforeFetchPatch,
		AfterFetchPatch,
		BeforeResponseHeaderPatch,
		BeforeInternalResponseHeaderPatch,
		AtResponseHeaderPatch,
		AfterInternalResponseHeaderPatch,
		AfterResponseHeaderPatch,
		BeforeResponseBodyPatch,
		BeforeResponseBodyRead,
		AfterResponseBodyRead,
		BeforeInternalResponseBodyPatch,
		AtResponseBodyPatch,
		AfterInternalResponseBodyPatch,
		BeforeResponseBodyWrite,
		AfterResponseBodyWrite,
		AfterResponseBodyPatch,
		AfterAllPatch
	};

	using patch_map_t = std::unordered_multimap<PatchTiming, patch_t>;

	// 对 url 进行 patch。总是假定 url 是一个合法的 url，即包括下面几种：
	//	  full (https only): https://xxxx
	//	  half: //xxxx
	//	  absolute: /xxxx
	//	  relative: xxx (will not check format)
	// 对于前两种格式，会检查 url 是否应该被代理
	// 对于不应该被代理或不符合格式的 url，会返回未 patch 的 url
	url_t url_patch(const Request&, const DomainStrategy&, const url_t&);

	// 对 patch 过后的 url 求原 url。同样是包括上述四种情况，但格式的检查会稍严格些（检查 host 和几级层 path 是否合理）
	// 若不符合格式，会返回传入的 url 而不报错
	url_t url_depatch(const Request&, const DomainStrategy&, const url_t&);

	// 对 setcookie 进行 patch
	std::vector<setcookie_t> setcookie_patch(const Request&, const DomainStrategy&, const setcookie_t&);

	// 向上游发出请求并回收响应
	std::unique_ptr<Response> fetch(std::unique_ptr<Request>, Synchronized<"mirror"_ss>::ShutdownCallbackHandler&);
}
