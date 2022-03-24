# pragma once
# include <mirism/server/utils/atomic.hpp>

namespace mirism::server::utils
{
	template<not_cvref_type T> inline
		Atomic<T>::Atomic
		(const T& v)
		: Value_(v)
		{}
	template<not_cvref_type T> inline
		Atomic<T>::Atomic
		(T&& v)
		: Value_(std::move(v))
		{}

	template<not_cvref_type T> template<detail_::AtomicReadFunction<T> ReadFunction> inline
		auto
		Atomic<T>::read
		(ReadFunction&& read_function) const
		-> std::invoke_result_t<ReadFunction&, const T&>
	{
		std::lock_guard<std::mutex> lock(Mutex_);
		return read_function(Value_);
	}
	template<not_cvref_type T>
		template<detail_::AtomicReadFunction<T> ReadFunction, detail_::AtomicWatchFunction<T> WatchFunction> inline
		auto
		Atomic<T>::read
		(ReadFunction&& read_function, WatchFunction&& watch_function) const
		-> std::invoke_result_t<ReadFunction&, const T&>
	{
		std::unique_lock<std::mutex> lock(Mutex_);
		ConditionVariable_.wait(lock, [this, &watch_function]{return watch_function(Value_);});
		return read_function(Value_);
	}
	template<not_cvref_type T>
		template<detail_::AtomicReadFunction<T> ReadFunction, detail_::AtomicWatchFunction<T> WatchFunction> inline
		auto
		Atomic<T>::read
		(ReadFunction&& read_function, WatchFunction&& watch_function, std::chrono::steady_clock::duration duration)
			const
		-> std::conditional_t
		<
			std::same_as<std::invoke_result_t<ReadFunction&, const T&>, void>,
			bool,
			std::optional<std::invoke_result_t<ReadFunction&, const T&>>
		>
	{
		std::unique_lock<std::mutex> lock(Mutex_);
		auto success = ConditionVariable_.wait_for
			(lock, duration, [this, &watch_function]{return watch_function(Value_);});
		using non_optional_return_type = decltype(read_function(Value_));
		if constexpr (std::same_as<non_optional_return_type, void>)
		{
			if (success)
			{
				read_function(Value_);
				return true;
			}
			else
				return false;
		}
		else
		{
			if (success)
				return std::make_optional(read_function(Value_));
			else
				return std::nullopt;
		}
	}
	template<not_cvref_type T> template<detail_::AtomicWriteFunction<T> WriteFunction> inline
		void
		Atomic<T>::write
		(WriteFunction&& write_function)
	{
		std::lock_guard<std::mutex> lock(Mutex_);
		write_function(Value_);
		ConditionVariable_.notify_all();
	}
	template<not_cvref_type T>
		template<detail_::AtomicWriteFunction<T> WriteFunction, detail_::AtomicWatchFunction<T> WatchFunction> inline
		void
		Atomic<T>::write
		(WriteFunction&& write_function, WatchFunction&& watch_function)
	{
		std::unique_lock<std::mutex> lock(Mutex_);
		ConditionVariable_.wait
			(lock, [this, &watch_function]{return watch_function(const_cast<const T&>(Value_));});
		write_function(Value_);
		ConditionVariable_.notify_all();
	}
	template<not_cvref_type T>
		template<detail_::AtomicWriteFunction<T> WriteFunction, detail_::AtomicWatchFunction<T> WatchFunction> inline
		bool
		Atomic<T>::write
		(WriteFunction&& write_function, WatchFunction&& watch_function, std::chrono::steady_clock::duration duration)
	{
		std::unique_lock<std::mutex> lock(Mutex_);
		auto success = ConditionVariable_.wait_for
			(lock, duration, [this, &watch_function]{return watch_function(const_cast<const T&>(Value_));});
		if (success)
		{
			write_function(Value_);
			ConditionVariable_.notify_all();
		}
		return success;
	}
	template<not_cvref_type T> template<detail_::AtomicWatchFunction<T> WatchFunction> inline
		void
		Atomic<T>::wait
		(WatchFunction&& watch_function) const
	{
		std::unique_lock<std::mutex> lock(Mutex_);
		ConditionVariable_.wait(lock, [this, &watch_function]{return watch_function(Value_);});
	}
	template<typename T> template<detail_::AtomicWatchFunction<T> WatchFunction> inline
		bool
		Atomic<T>::wait
		(WatchFunction&& watch_function, std::chrono::steady_clock::duration duration) const
	{
		std::unique_lock<std::mutex> lock(Mutex_);
		return ConditionVariable_.wait_for(lock, duration, [this, &watch_function]{return watch_function(Value_);});
	}
}
