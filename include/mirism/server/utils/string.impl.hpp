# pragma once
# include <mirism/logger.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::utils::string
{
	template<> inline
		std::string
		compress<gzip>
		(std::string data)
	{
		Logger::Guard log;
		try
		{
			std::stringstream compressed;
			std::stringstream origin(std::move(data));
			boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
			out.push
			(
				boost::iostreams::gzip_compressor
				(boost::iostreams::gzip_params(boost::iostreams::gzip::best_compression))
			);
			out.push(origin);
			boost::iostreams::copy(out, compressed);
			return std::move(compressed).str();
		}
		catch (...)
		{
			log.info("compress gzip failed");
			return "";
		}
	}
	template<> inline
		std::string
		compress<brotli>
		(std::string data)
	{
		static_assert(sizeof(uint8_t) == sizeof(char));
		Logger::Guard log;
		try
		{
			auto instance = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
			std::array<uint8_t, BROTLI_BUFFER_SIZE> buffer;
			std::string result;

			size_t available_in = data.length(), available_out = buffer.size();
			const uint8_t* next_in = reinterpret_cast<const uint8_t*>(data.c_str());
			uint8_t* next_out = buffer.data();

			do
			{
				auto compress_status = BrotliEncoderCompressStream
				(
					instance, BROTLI_OPERATION_FINISH,
					&available_in, &next_in, &available_out, &next_out, nullptr
				);
				if (!compress_status)
				{
					log.info("brotli compress failed");
					BrotliEncoderDestroyInstance(instance);
					return "";
				}
				result.append(reinterpret_cast<const char*>(buffer.data()), buffer.size() - available_out);
				available_out = buffer.size();
				next_out = buffer.data();
			}
			while (!(available_in == 0 && BrotliEncoderIsFinished(instance)));

			BrotliEncoderDestroyInstance(instance);
			return result;
		}
		catch (...)
		{
			log.info("compress brotli failed");
			return "";
		}
	}
	template<> inline
		std::string
		compress<deflated>
		(std::string data)
	{
		Logger::Guard log;
		try
		{
			std::stringstream compressed;
			std::stringstream origin(std::move(data));
			boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
			out.push
			(
				boost::iostreams::gzip_compressor
				(boost::iostreams::gzip_params(boost::iostreams::gzip::best_compression))
			);
			out.push(origin);
			boost::iostreams::copy(out, compressed);
			return std::move(compressed).str();
		}
		catch (...)
		{
			log.info("compress deflated failed");
			return "";
		}
	}

	template<> inline
		std::string
		decompress<gzip>
		(std::string data)
	{
		Logger::Guard log;
		try
		{
			std::stringstream decompressed;
			std::stringstream origin(std::move(data));
			boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
			out.push(boost::iostreams::gzip_decompressor());
			out.push(origin);
			boost::iostreams::copy(out, decompressed);
			return std::move(decompressed).str();
		}
		catch (...)
		{
			log.info("decompress gzip failed");
			return "";
		}
	}
	template<> inline
		std::string
		decompress<brotli>
		(std::string data)
	{
		static_assert(sizeof(uint8_t) == sizeof(char));
		Logger::Guard log;
		try
		{
			auto instance = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
			std::array<uint8_t, BROTLI_BUFFER_SIZE> buffer;
			std::string result;

			size_t available_in = data.length(), available_out = buffer.size();
			const uint8_t* next_in = reinterpret_cast<const uint8_t*>(data.data());
			uint8_t* next_out = buffer.data();
			BrotliDecoderResult oneshot_result;

			do
			{
				oneshot_result = BrotliDecoderDecompressStream
				(
					instance,
					&available_in, &next_in, &available_out, &next_out, nullptr
				);
				if
				(
					oneshot_result == BROTLI_DECODER_RESULT_ERROR
					|| oneshot_result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT
				)
				{
					log.info("brotli decompress failed");
					BrotliDecoderDestroyInstance(instance);
					return "";
				}
				result.append(reinterpret_cast<const char*>(buffer.data()), buffer.size() - available_out);
				available_out = buffer.size();
				next_out = buffer.data();
			}
			while (!(available_in == 0 && oneshot_result == BROTLI_DECODER_RESULT_SUCCESS));

			BrotliDecoderDestroyInstance(instance);
			return result;
		}
		catch (...)
		{
			log.info("decompress brotli failed");
			return "";
		}
	}

	inline
		std::string
		replace
		(
			const std::string& content, const std::regex& regex,
			std::function<std::string(const std::smatch&)> function
		)
	{
		auto find = [&content, &regex] -> cppcoro::generator<std::pair<std::string_view, std::sregex_iterator>>
		{
			auto unmatched_prefix_begin = content.cbegin();
			decltype(unmatched_prefix_begin) unmatched_prefix_end;
			std::sregex_iterator regit;
			while (true)
			{
				if (regit == std::sregex_iterator())
					regit = std::sregex_iterator(content.begin(), content.end(), regex);
				else
					regit++;
				if (regit == std::sregex_iterator())
					unmatched_prefix_end = content.end();
				else
					unmatched_prefix_end = (*regit)[0].first;
				co_yield
				{
					std::string_view
					(
						&*unmatched_prefix_begin,
						std::distance(unmatched_prefix_begin, unmatched_prefix_end)
					),
					regit
				};
				if (regit == std::sregex_iterator())
					break;
				unmatched_prefix_begin = (*regit)[0].second;
			}
		};

		Logger::Guard log;
		log.debug("receive string in address {}"_f(fmt::ptr(content.c_str())));
		std::string result;
		result.reserve(content.length() * 2);
		for (auto matched : find())
		{
			result.append(matched.first);
			if (matched.second != std::sregex_iterator())
				result.append(function(*matched.second));
		}
		return result;
	}

	inline
		std::string
		lowwer
		(std::string content)
	{
		for (auto& c : content)
			if (c >= 'A' && c <= 'Z')
				c += 'a' - 'A';
		return content;
	}
	inline
		std::string
		clean_host
		(const std::string& host)
	{
		auto pos1 = host.find_first_not_of('.'), pos2 = host.find_last_not_of('.');
		if (pos1 == std::string::npos)
			return "";
		if (pos2 >= pos1)
			return host.substr(pos1, pos2 - pos1 + 1);
		else
			return "";
	}
	inline
		cppcoro::generator<std::string_view>
		split(std::string str, char de)
	{
		std::string::size_type start_pos = 0;
		while (true)
		{
			if (auto pos = str.find(de, start_pos); pos != std::string::npos)
			{
				co_yield {std::next(str.begin(), start_pos), std::next(str.begin(), pos)};
				start_pos = pos + 1;
			}
			else
				break;
		}
		co_yield {std::next(str.begin(), start_pos), str.end()};
	};
}
