# pragma once
# include <mirism/common.hpp>

template <typename Char, Char... c> inline consteval
	mirism::StaticString<Char, c...>
	mirism::literals::operator ""_ss
	()
	{return {};}

template <typename Char, Char... c> template <typename... Param> inline
	std::string
	mirism::detail_::FormatLiteralHelper<Char, c...>::operator()
	(Param&&... param) const
	{return fmt::format(StaticString<Char, c...>::StringView, std::forward<Param>(param)...);}

template <typename Char, Char... c> inline consteval
	mirism::detail_::FormatLiteralHelper<Char, c...>
	mirism::literals::operator ""_f
	()
	{return {};}

template <typename T1, typename T2> consteval inline
	bool
	mirism::detail_::is_specialization_of_detail_::params_is_ok
	()
{
	if constexpr (std::tuple_size_v<T1> == 0)
		return true;
	else if constexpr (std::tuple_size_v<T2> == 0)
		return false;
	else if constexpr (std::same_as<std::tuple_element_t<0, T1>, std::tuple_element_t<0, T2>>)
		return params_is_ok
			<typename DropFirstMemberOfTuple<T1>::type, typename DropFirstMemberOfTuple<T2>::type>();
	else
		return false;
}

template <typename T, template <typename...> typename Template> template <typename... ProvidedArgs> consteval inline
	bool
	mirism::detail_::is_specialization_of_detail_::Helper<T, Template>::check_provided_args
	()
	{return false;}

template <template <typename...> typename Template, typename... Args> template <typename... ProvidedArgs>
	consteval inline
	bool
	mirism::detail_::is_specialization_of_detail_::Helper<Template<Args...>, Template>::check_provided_args
	()
	{return params_is_ok<std::tuple<ProvidedArgs...>, std::tuple<Args...>>();}

template<typename T> inline constexpr
	auto
	mirism::detail_::FormatterReuseProxy<T>::parse
	(fmt::format_parse_context& ctx)
	-> std::invoke_result_t<decltype(&fmt::format_parse_context::begin), fmt::format_parse_context>
{
	if (ctx.begin() != ctx.end() && *ctx.begin() != '}')
		throw fmt::format_error
		(
			"{} do not support to be format, so the wrapper should not have any format syntax."_f
				(nameof::nameof_full_type<T>())
		);
	return ctx.begin();
}

template<mirism::detail_::OptionalWrap Wrap> template<typename FormatContext> inline
	auto
	fmt::formatter<Wrap>::format
	(const Wrap& wrap, FormatContext& ctx)
	-> std::invoke_result_t<decltype(&FormatContext::out), FormatContext>
{
	static_assert(std::same_as<typename FormatContext::char_type, char>);
	using namespace mirism::literals;
	using value_t = typename mirism::detail_::non_cv_value_type<Wrap>::type;
	auto format_value_type = [&, this](const value_t& value)
	{
		// 指针需要特殊处理：指针可以使用 stream 输出，但 fmt 不允许除了 [c][v] void* 和 [c] char* 之外的指针
		// 对于这种情况，我们认为不可 format
		if constexpr
		(
			std::is_pointer_v<value_t>
			&& !std::same_as<std::remove_cvref_t<std::remove_pointer_t<value_t>>, void>
			&& !std::same_as
			<
				std::remove_reference_t<std::remove_const_t<std::remove_pointer_t<value_t>>>,
				typename FormatContext::char_type
			>
		)
			return fmt::format_to(ctx.out(), "non-null unformattable value");
		else if constexpr (std::default_initializable<fmt::formatter<value_t>>)
			mirism::detail_::FormatterReuseProxy<value_t>::format(value, ctx);
		else if constexpr
		(
			requires(std::ostream& os, const value_t& val)
			{
				{os << val} -> std::same_as<std::ostream&>;
			}
		)
			format_to(ctx.out(), "{}", value);
		else
			format_to(ctx.out(), "non-null unformattable value");
	};
	format_to(ctx.out(), "(");
	if constexpr (mirism::is_specialization_of<Wrap, std::optional>)
	{
		if (wrap)
			format_value_type(*wrap);
		else
			format_to(ctx.out(), "null");
	}
	else if constexpr (mirism::is_specialization_of<Wrap, std::weak_ptr>)
	{
		if (auto shared = wrap.lock())
		{
			format_to(ctx.out(), "{} ", ptr(shared.get()));
			format_value_type(*shared);
		}
		else
			format_to(ctx.out(), "null");
	}
	else
	{
		if (wrap)
		{
			format_to(ctx.out(), "{} ", ptr(wrap.get()));
			format_value_type(*wrap);
		}
		else
			format_to(ctx.out(), "null");
	}
	return format_to(ctx.out(), ")");
}

template<typename T, typename Ret, typename... Params> template<auto F> inline
	auto
	mirism::detail_::PSMFHelper<Ret(T::*)(Params...) const>::generate_psmf
	()
	-> psmf_type
{
	return [](Params... ps) -> Ret
	{
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wpmf-conversions"
		return pmf_type(F)(reinterpret_cast<const T*>(NULL), std::forward<Params>(ps)...);
# pragma GCC diagnostic pop
	};
}

namespace mirism
{
	template<typename... Ts> inline
		std::size_t
		hash
		(Ts&&... ps)
	{
		std::size_t result = 0;
		(boost::hash_combine(result, ps), ...);
		return result;
	}
	[[gnu::always_inline]] inline
		void
		unused
		(auto&&...) 
		{}
	inline
		std::regex
		literals::operator""_re
		(const char* s, std::size_t len)
		{return std::regex(s, len);}
}
inline
	std::ostream&
	nghttp2::asio_http2::operator<<
	(std::ostream& os, const header_value& v)
{
	using namespace mirism::literals;
	return os << "{{{}, \"{}\"}}"_f(v.sensitive, v.value);
};
