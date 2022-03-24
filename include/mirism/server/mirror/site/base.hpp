# pragma once
# include <mirism/server/mirror/content/base.hpp>

namespace mirism::server::mirror::site
{
	template<typename Requests = void, typename Responses = void>
	class Base;
	template<>
	class Base<void, void>
	{
	public:
		using content_type_t = std::string;
		using content_map_t = std::unordered_map<content_type_t, std::function<std::unique_ptr<content::Base>()>>;

		virtual inline
		~Base() = default;

		Base();

		// 处理一个请求。函数可以接受 nullptr，也可能返回 nullptr。接受到 nullptr 时，会立即返回 nullptr
		// 函数会在收到完整的响应头后返回，响应体稍后异步写入到 pipe
		virtual
		std::unique_ptr<Response> operator()
		(
			std::unique_ptr<Request>, const patch_map_t&,
			const DomainStrategy&, Synchronized<"mirror"_ss>::ShutdownCallbackHandler&
		);

		// 获取该类对应的所有 Group
		virtual
		const DomainStrategy& get_domain_strategy() const = 0;

		// 获取该类使用的所有 content
		virtual
		const content_map_t& get_request_content() const = 0;
		virtual
		std::unique_ptr<content::Base> get_default_request_content() const;
		virtual
		const content_map_t& get_response_content() const = 0;
		virtual
		std::unique_ptr<content::Base> get_default_response_content() const;

	protected:

		// 派生类在构造函数中将 patch 写入这里
		patch_map_t PatchMap_;

		template<std::derived_from<content::Base>... Ts>
		static content_map_t create_content_map_();
	};
	template<std::derived_from<content::Base>... Ts>
	class Base<std::tuple<Ts...>, void> : public virtual Base<void, void>
	{
	public:
		const content_map_t& get_request_content() const override;
	};
	template<std::derived_from<content::Base>... Ts>
	class Base<void, std::tuple<Ts...>> : public virtual Base<void, void>
	{
	public:
		const content_map_t& get_response_content() const override;
	};
	template<typename Requests, typename Responses>
	class Base : public virtual Base<Requests, void>, public virtual Base<void, Responses>
	{};
}
