# pragma once
# include <mirism/server/mirror/content/base.hpp>

namespace mirism::server::mirror::content
{
	inline
		Base&
		Base::write
		(const encoding_t& encoding, const std::shared_ptr<utils::Pipe>& pipe)
	{
		Logger::Guard log(encoding, pipe, fmt::ptr(this));
		Pipe_ = pipe;
		return *this;
	}
	inline
		std::shared_ptr<utils::Pipe>
		Base::read
		()
	{
		Logger::Guard log(fmt::ptr(this));
		auto pp = Pipe_;
		Pipe_ = nullptr;
		return log.rtn(std::move(pp));
	}
	inline
		Base&
		Base::patch
		(std::function<void(content_t&)>)
	{
		Logger::Guard log(fmt::ptr(this));
		log.info("ignore patch on Base");
		return *this;
	}
	inline
		Base&
		Base::patch
		(const Request&, const DomainStrategy&)
	{
		Logger::Guard log(fmt::ptr(this));
		return *this;
	}
	inline
		Base&
		Base::depatch
		(const Request&, const DomainStrategy&)
	{
		Logger::Guard log(fmt::ptr(this));
		return *this;
	}
	inline
		const Base::type_set_t&
		Base::get_type_set
		() const
	{
		static type_set_t type_set;
		return type_set;
	}
}
