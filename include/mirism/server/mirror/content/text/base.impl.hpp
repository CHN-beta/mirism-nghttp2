# pragma once
# include <mirism/server/mirror/content/text/base.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::mirror::content::text
{
	inline
		Base&
		Base::write
		(const encoding_t& encoding, const std::shared_ptr<utils::Pipe>& pipe)
	{
		Logger::Guard log(encoding, pipe, fmt::ptr(this));
		if (pipe)
		{
			if (auto content = pipe->read_all(10s))
				Content_ = *std::move(content);
			else
			{
				log.info("read from pipe failed");
				Content_ = "";
			}
		}
		else
		{
			log.debug("pipe is nullptr");
			Content_ = "";
		}
		Encoding_ = encoding;
		if (encoding)
		{
			if (*encoding == "gzip")
				Content_ = utils::string::decompress<utils::string::gzip>(std::move(Content_));
			else if (*encoding == "br")
				Content_ = utils::string::decompress<utils::string::brotli>(std::move(Content_));
			else
				log.info("unsupported encoding \"{}\", read as normal"_f(*encoding));
		}
		return *this;
	}
	inline
		std::shared_ptr<utils::Pipe>
		Base::read
		()
	{
		Logger::Guard log(fmt::ptr(this));
		if (Encoding_)
		{
			if (*Encoding_ == "gzip")
				Content_ = utils::string::compress<utils::string::gzip>(std::move(Content_));
			else if (*Encoding_ == "br")
				Content_ = utils::string::compress<utils::string::brotli>(std::move(Content_));
			else
				log.info("unsupported encoding \"{}\", will not compress"_f(*Encoding_));
		}
		auto rtn = std::make_shared<utils::Pipe>();
		log.fork
		(
			[rtn, content = std::move(Content_)]
			{
				Logger::Guard log;
				log.debug("start thread {}"_f(std::hash<std::thread::id>()(std::this_thread::get_id())));
				rtn->write(content, 10s);
				log.debug("stop thread {}"_f(std::hash<std::thread::id>()(std::this_thread::get_id())));
			}
		);
		return log.rtn(std::move(rtn));
	}
	inline
		std::string
		Base::patch
		(const std::string content, const Request&, const DomainStrategy&)
		{return content;}
	inline
		std::string
		Base::patch_virtual
		(const std::string content, const Request& request, const DomainStrategy& domain_strategy) const
		{return patch(std::move(content), request, domain_strategy);}
	inline
		Base&
		Base::patch
		(const Request& request, const DomainStrategy& domain_strategy)
	{
		Logger::Guard log(fmt::ptr(this));
		Content_ = patch_virtual(std::move(Content_), request, domain_strategy);
		return *this;
	}
	inline
		Base&
		Base::patch
		(std::function<void(content_t&)> function)
	{
		Logger::Guard log(fmt::ptr(this));
		function(Content_);
		return *this;
	}
}
