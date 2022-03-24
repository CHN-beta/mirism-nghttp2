# pragma once
# include <mirism/server/utils/pipe.hpp>

namespace mirism::server::utils
{
	inline
		Pipe::Pipe
		()
		: Data_({false, false, {}})
		{}
	inline
		bool
		Pipe::write
		(const std::string_view& data, std::chrono::steady_clock::duration time)
	{
		Logger::Guard log(fmt::ptr(this));
		bool write_result = Data_.write
		(
			[&data](DataType_& internal_data)
			{
				if (!internal_data.End)
				{
					internal_data.Data = data;
					internal_data.ReadyToRead = true;
				}
			},
			[](const DataType_& internal_data){return internal_data.End || !internal_data.ReadyToRead;},
			time
		);
		if (write_result)
		{
			bool wait_result = Data_.write
			(
				[](DataType_& internal_data){internal_data.End = true;},
				[](const DataType_& internal_data){return internal_data.End || !internal_data.ReadyToRead;},
				time
			);
			if (wait_result)
				return true;
			else
				log.info("pipe {} write wait failed"_f(fmt::ptr(this)));
		}
		else
			log.info("pipe {} write failed"_f(fmt::ptr(this)));
		shutdown();
		return false;
	}
	inline
		nghttp2::asio_http2::data_cb
		Pipe::write
		(std::chrono::steady_clock::duration time)
	{
		Logger::Guard log(fmt::ptr(this));
		return [pp = shared_from_this(), time](const std::uint8_t* data, std::size_t len)
		{
			Logger::Guard log(pp, len);
			bool write_result = pp->Data_.write
			(
				[&data, &len, &log](DataType_& internal_data)
				{
					if (!internal_data.End)
					{
						if (len != 0)
						{
							internal_data.Data = std::string_view(reinterpret_cast<const char*>(data), len);
							internal_data.ReadyToRead = true;
							log.debug("write {}"_f(len));
						}
						else
						{
							internal_data.End = true;
							log.debug("write eof");
						}
					}
				},
				[](const DataType_& internal_data){return internal_data.End || !internal_data.ReadyToRead;},
				time
			);
			if (write_result)
			{
				bool wait_result = pp->Data_.wait
				(
					[](const DataType_& internal_data){return internal_data.End || !internal_data.ReadyToRead;},
					time
				);
				if (!wait_result)
				{
					log.info("write wait timeout.");
					pp->shutdown();
				}
			}
			else
			{
				log.info("write timeout.");
				pp->shutdown();
			}
		};
	}
	inline
		std::optional<std::string>
		Pipe::read_all
		(std::chrono::steady_clock::duration time)
	{
		Logger::Guard log(fmt::ptr(this));
		std::string result;
		bool end = false;
		while (!end)
		{
			bool read_result = Data_.write
			(
				[&result, &end, &log](DataType_& internal_data)
				{
					if (internal_data.End)
						end = true;
					else
					{
						result.append(internal_data.Data);
						log.debug("read {} in once"_f(internal_data.Data.length()));
						internal_data.ReadyToRead = false;
					}
				},
				[](const DataType_& internal_data){return internal_data.End || internal_data.ReadyToRead;},
				time
			);
			if (!read_result)
			{
				log.info("pipe {} read failed"_f(fmt::ptr(this)));
				shutdown();
				return std::nullopt;
			}
		}
		log.debug("read {} in total"_f(result.length()));
		return result;
	}
	inline
		nghttp2::asio_http2::generator_cb
		Pipe::read
		(std::chrono::steady_clock::duration time)
	{
		Logger::Guard log(fmt::ptr(this));
		return [pp = shared_from_this(), time](std::uint8_t* data, std::size_t len, std::uint32_t* data_flags)
		{
			static_assert(sizeof(std::uint8_t) == sizeof(char));
			Logger::Guard log(pp, len);
			std::size_t have_read = 0;
			bool end = false;
			while (len > 0 && !end)
			{
				bool read_result = pp->Data_.write
				(
					[&data, &len, &have_read, &end, &log](DataType_& internal_data)
					{
						if (internal_data.End)
						{
							log.debug("read eof");
							end = true;
						}
						else
						{
							std::size_t read_size = std::min(len, internal_data.Data.length());
							log.debug("read {}"_f(read_size));
							std::memcpy(data, internal_data.Data.data(), read_size);
							len -= read_size;
							internal_data.Data.remove_prefix(read_size);
							have_read += read_size;
							data += read_size;
							if (internal_data.Data.length() == 0)
							{
								log.debug("pipe empty now");
								internal_data.ReadyToRead = false;
							}
						}
					},
					[](const DataType_& internal_data){return internal_data.End || internal_data.ReadyToRead;},
					time
				);
				if (!read_result)
				{
					log.info("pipe read failed.");
					end = true;
				}
			}
			if (end)
				*data_flags |= NGHTTP2_DATA_FLAG_EOF;
			return have_read;
		};
	}
	inline
		bool
		Pipe::end
		() const
	{
		return Data_.read([](const DataType_& internal_data){return internal_data.End;});
	}
	inline
		bool
		Pipe::empty
		() const
	{
		return Data_.read
		(
			[](const DataType_& internal_data){return internal_data.End;},
			[](const DataType_& internal_data){return internal_data.End || internal_data.ReadyToRead;}
		);
	}
	inline
		std::optional<bool>
		Pipe::empty
		(std::chrono::steady_clock::duration time) const
	{
		return Data_.read
		(
			[](const DataType_& internal_data){return internal_data.End;},
			[](const DataType_& internal_data){return internal_data.End || internal_data.ReadyToRead;},
			time
		);
	}
	inline
		void
		Pipe::shutdown
		()
	{
		Logger::Guard log(fmt::ptr(this));
		Data_.write([](DataType_& internal_data){internal_data.End = true;});
	}
}
