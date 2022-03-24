# pragma once

# include <string>
# include <regex>
# include <sstream>
# include <stop_token>
# include <iostream>
# include <unordered_set>
# include <experimental/memory>
# include <atomic>
# include <chrono>
# include <thread>
# include <mutex>
# include <shared_mutex>
# include <ranges>
# include <cstdint>
# include <fstream>
# include <condition_variable>
# include <syncstream>

# include <nghttp2/asio_http2_server.h>
# include <nghttp2/asio_http2_client.h>

# define BOOST_STACKTRACE_USE_BACKTRACE
# include <boost/stacktrace.hpp>

# include <boost/iostreams/filtering_streambuf.hpp>
# include <boost/iostreams/copy.hpp>
# include <boost/iostreams/filter/gzip.hpp>
# include <boost/iostreams/filter/zlib.hpp>

# include <brotli/encode.h>
# include <brotli/decode.h>
# ifndef BROTLI_BUFFER_SIZE
#   define BROTLI_BUFFER_SIZE 1024
# endif

# include <fmt/format.h>
# include <fmt/ranges.h>
# include <fmt/chrono.h>
# include <fmt/ostream.h>

# include <nameof.hpp>

# include <boost/container_hash/hash.hpp>

# include <cppcoro/generator.hpp>

# include <boost/range.hpp>

# include <tgbot/Bot.h>

# include <date/date.h>

# ifdef MIRISM_DEBUG
#   define MIRISM_VERBOSE
#   define MIRISM_OBJECT_INDEX
#   define MIRISM_THREAD_INDEX
# endif
namespace mirism::detail_
{
# ifdef MIRISM_VERBOSE
	constexpr bool MirismVerbose = true;
# else
	constexpr bool MirismVerbose = false;
# endif
# ifdef MIRISM_OBJECT_INDEX
	constexpr bool MirismObjectIndex = true;
# else
	constexpr bool MirismObjectIndex = false;
# endif
# ifdef MIRISM_THREAD_INDEX
	constexpr bool MirismThreadIndex = true;
# else
	constexpr bool MirismThreadIndex = false;
# endif
}

// 存储编译期可以使用的字符串（可以作为模板参数）
namespace mirism
{
	template <typename Char, Char... c>
		struct StaticString
	{
		static constexpr
			std::array<Char, sizeof...(c)>
			Array
			{c...};
		static constexpr
			std::basic_string_view<Char>
			StringView
			{Array.data(), sizeof...(c)};
	};
	inline namespace literals
	{
		template <typename Char, Char... c> consteval
			StaticString<Char, c...>
			operator""_ss
			();
	}
}

// 带编译期格式化字符串检查的 operator""_format
namespace mirism
{
	namespace detail_
	{
		template <typename Char, Char... c>
			struct FormatLiteralHelper
			: protected StaticString<Char, c...>
		{
			template <typename... Param>
				std::string
				operator()
				(Param&&... param) const;
		};
	}
	inline namespace literals
	{
		template <typename Char, Char... c> consteval
			detail_::FormatLiteralHelper<Char, c...>
			operator""_f
			();
	}
}

// 一些可能有用的 concept
namespace mirism
{
	template <typename T>
		concept not_cvref_type
		= std::same_as<T, std::remove_cvref_t<T>>;

	namespace detail_::is_specialization_of_detail_
	{
		template <typename T>
			struct DropFirstMemberOfTuple;
		template <typename T, typename... Ts>
			struct DropFirstMemberOfTuple<std::tuple<T, Ts...>>
			{
				using type = std::tuple<Ts...>;
			};

		template <typename T1, typename T2> consteval
			bool
			params_is_ok
			();

		template <typename T, template <typename...> typename Template>
			struct Helper
			{
				template <typename... ProvidedArgs> consteval static
				 	bool
					check_provided_args
					();
			};
		template <template <typename...> typename Template, typename... Args>
			struct Helper<Template<Args...>, Template>
			{
				template <typename... ProvidedArgs> consteval static
				 	bool
					check_provided_args
					();
			};
	}
	template <typename T, template <typename...> typename Template, typename... ProvidedArgs>
		concept is_specialization_of
		= detail_::is_specialization_of_detail_::Helper<T, Template>::template check_provided_args<ProvidedArgs...>();

	namespace detail_
	{
		template <typename, typename = void>
			struct CompleteTypeHelper
			: std::false_type
			{};
		template<typename T>
			struct CompleteTypeHelper<T, std::void_t<decltype(sizeof(T))>>
			: std::true_type
			{};
	}
	template <typename T>
		concept complete_type
		= detail_::CompleteTypeHelper<T>::value;

	template <typename From, typename To>
		concept implicitly_convertible_to
		= std::is_convertible<From, To>::value;
	template <typename To, typename From>
		concept implicitly_convertible_from
		= std::is_convertible<From, To>::value;
	template <typename From, typename To>
		concept explicitly_convertible_to
		= std::is_constructible<To, From>::value;
	template <typename To, typename From>
		concept explicitly_convertible_from
		= std::is_constructible<To, From>::value;
	template <typename From, typename To>
		concept convertible_to
		= implicitly_convertible_to<From, To> || explicitly_convertible_to<From, To>;
	template <typename To, typename From>
		concept convertible_from
		= convertible_to<From, To>;

	// 在编译器，尝试使用默认构造函数构造 Function 类的一个对象，并用这些参数调用它的 operator()
	// 如果可以成功，则满足约束，否则不满足
	template <typename Function, auto... param>
		concept consteval_invokable
		= requires()
		{
			typename std::type_identity_t<int[(Function()(param...), 1)]>;
		};

	// 限定类型的编译时字符串
	namespace detail_
	{
		template <typename C, typename T>
			struct IsStaticStringHelper : std::false_type
			{};
		template <typename C, C... c, typename T>
			struct IsStaticStringHelper<C, StaticString<T, c...>> : std::true_type
			{};
	}
	template <typename T, typename C>
		concept is_static_string
		= detail_::IsStaticStringHelper<C, T>::value;
}

// 允许智能指针和 std::optional 被格式化输出
namespace mirism
{
	namespace detail_
	{
		// 定义所有需要允许格式化的新类型
		template <typename T>
			concept OptionalWrap
			= is_specialization_of<T, std::optional>
				|| is_specialization_of<T, std::shared_ptr>
				|| is_specialization_of<T, std::weak_ptr>
				|| is_specialization_of<T, std::unique_ptr>
				|| is_specialization_of<T, std::experimental::observer_ptr>;

		// 方便地取得 value_type 或 element_type
		template <typename T>
			struct non_cv_value_type
			{};
		template <OptionalWrap T> requires requires() {typename T::value_type;}
			struct non_cv_value_type<T>
		{
			using type = std::remove_cvref_t<typename T::value_type>;
		};
		template <OptionalWrap T> requires requires() {typename T::element_type;}
			struct non_cv_value_type<T>
		{
			using type = std::remove_cvref_t<typename T::element_type>;
		};

		// 若 value_type 或者 element_type 本身是可以格式化的，那么需要重用它们的 fmt::formatter 以解析格式；
		// 否则，写一个自定义的解析格式的函数，不允许任何格式化参数
		template <typename T>
			struct FormatterReuseProxy
		{
			constexpr
				auto
				parse
				(fmt::format_parse_context&)
				-> std::invoke_result_t<decltype(&fmt::format_parse_context::begin), fmt::format_parse_context>;
		};
		template <typename T>
			requires (!is_specialization_of<T, std::weak_ptr> && std::default_initializable<fmt::formatter<T>>)
			struct FormatterReuseProxy<T>
			: fmt::formatter<T>
			{};
	}
}

// 最终定义所需的 formatter
template <mirism::detail_::OptionalWrap Wrap>
	struct fmt::formatter<Wrap>
	: mirism::detail_::FormatterReuseProxy<typename mirism::detail_::non_cv_value_type<Wrap>::type>
{
	template <typename FormatContext>
		auto
		format(const Wrap&, FormatContext&)
		-> std::invoke_result_t<decltype(&FormatContext::out), FormatContext>;
};

// pointer of static member function
// 因为 C++ 不支持 virtual static function，
// 即一个成员函数要么不能在运行时查虚函数表获得，要么不能在不构造对象的情况下合法使用
// （如果直接使用函数指针并传入 NULL，有时编译器还会要求去查虚函数表，导致出错）
// 因此，这里定义一个函数，用来模拟 virtual static function
namespace mirism
{
	namespace detail_
	{
		template <typename T>
			struct PSMFHelper;
		template <typename T, typename Ret, typename... Params>
			struct PSMFHelper<Ret(T::*)(Params...) const>
		{
			using pmf_type = Ret(*)(const T*, Params...);
			using psmf_type = Ret(*)(Params...);
			template<auto F> static
				psmf_type
				generate_psmf
				();
		};
	}
	template<auto F> inline
		auto
		psmf
		= detail_::PSMFHelper<decltype(F)>::template generate_psmf<F>;
}

namespace mirism
{
	template<typename... Ts>
		std::size_t
		hash
		(Ts&&...);
	void
		unused
		(auto&&...);
	inline namespace literals
	{
		using namespace std::literals;
		using namespace fmt::literals;
		std::regex
			operator""_re
			(const char*, std::size_t);
	}
}
namespace nghttp2::asio_http2
{
	std::ostream&
		operator<<
		(std::ostream&, const header_value&);
}
