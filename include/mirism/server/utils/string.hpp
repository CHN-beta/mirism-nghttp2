# pragma once
# include <mirism/common.hpp>

namespace mirism::server::utils::string
{
	enum CompressMethod
	{
		gzip,
		brotli,
		deflated
	};

	using mirism_host_t = std::string;
	using host_t = std::string;
	using command_t = std::string;
	using group_t = std::string;
	using url_t = std::string;
	using host_set_t = std::unordered_set<std::string>;
	using setcookie_t = std::string;
	using setcookie_vector_t = std::vector<std::string>;

	// 压缩字符串，如果出错则返回空字符串
	template<CompressMethod>
		std::string
		compress
		(std::string);
	template<>
		std::string
		compress<CompressMethod::gzip>
		(std::string);
	template<>
		std::string
		compress<CompressMethod::brotli>
		(std::string);
	template<>
		std::string
		compress<CompressMethod::deflated>
		(std::string);

	// 解压字符串，如果出错则返回空字符串
	template<CompressMethod>
		std::string
		decompress
		(std::string);
	template<>
		std::string
		decompress<CompressMethod::gzip>
		(std::string);
	template<>
		std::string
		decompress<CompressMethod::brotli>
		(std::string);

	// 按照指定的正则表达式在字符串中搜索，并调用回调函数处理匹配的结果
	// 例如： replace("12bcd33", "[a-z]"_re, [](const std::smatch& match){return match.str() + "f";})
	// 这个调用将所有的小写字母后都增加了一个f，得到"12bfcfdf33"
	std::string
		replace
		(const std::string&, const std::regex&, std::function<std::string(const std::smatch&)>);

	// 英文字母全部转为小写
	std::string
		lowwer
		(std::string);

	// 删除 host 前后多余的点
	std::string
		clean_host
		(const std::string&);

	// 将字符串分割开来
	cppcoro::generator<std::string_view>
		split
		(std::string, char);
}
