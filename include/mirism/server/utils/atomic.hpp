# pragma once
# include <mirism/common.hpp>

namespace mirism::server::utils
{
	// 在不太计较性能的前提下，方便地实现一些原子化的操作
	// 成员函数涉及到的模板参数应该有下面格式的 operator()：（通常来说，使用 lambda 表达式就可以）
	//	  * WatchFunction<T>: watch_function(const T&) -> boolean_testable
	//		  指示是否满足要求的条件，在满足条件后再调用 ReadFunction 去读取数据或 ReadFunction 去写入数据，若满足则返回 true
	//	  * ReadFunction<T> : read_function(const T&) -> not_cvref_type
	//		  读取数据，返回读取到的值，当然也可以按引用捕获局部变量然后直接将值存储进去
	//	  * WriteFunction<T>: write_function(T&) -> anything
	//		  修改值，通常应该使用 lambda 捕获局部变量以供调用时写入，返回值会被忽略
	// 除了构造函数和析构函数以外，对成员函数的调用（读取或写入）会被阻塞，直到：
	//	  * 若没有传入等待时间，则直到读取或写入成功，返回 void 或读取到的值；
	//	  * 若传入了等待时间，则直到读取或写入成功（返回读取到的值或 true），或者超时（返回 false 或者 std::nullopt）
	// 在设计和使用该模板时，总是假定 WatchFunction / ReadFunction / WriteFunction 执行所需要的时间极短，可以忽略不计；
	//	  也即，任何一个线程都不应该长时间占用 Mutex_

	namespace detail_
	{
		template<typename ReadFunctionType, typename T>
			concept AtomicReadFunction
			=	requires(ReadFunctionType&& read_function, const T& val){{read_function(val)} -> not_cvref_type;}
				&& not_cvref_type<T>;
		template<typename WriteFunctionType, typename T>
			concept AtomicWriteFunction
			=	requires(WriteFunctionType&& write_function, T& val){write_function(val);}
				&& not_cvref_type<T>;
		template<typename WatchFunctionType, typename T>
			concept AtomicWatchFunction
			=	requires(WatchFunctionType&& watch_function, T& val)
				{
					// {watch_function(val)} -> std::boolean_testable;
					static_cast<bool>(watch_function(val));
					static_cast<bool>(!watch_function(val));
				}
				&& not_cvref_type<T>;
	}

	template<not_cvref_type T>
		class Atomic
	{
		public:
			explicit
				Atomic
				(const T&);
			explicit
				Atomic
				(T&&);
			Atomic
				()
				= default;

			template<detail_::AtomicReadFunction<T> ReadFunction>
				auto
				read
				(ReadFunction&&) const
				-> std::invoke_result_t<ReadFunction&, const T&>;
			template<detail_::AtomicReadFunction<T> ReadFunction, detail_::AtomicWatchFunction<T> WatchFunction>
				auto
				read
				(ReadFunction&&, WatchFunction&&) const
				-> std::invoke_result_t<ReadFunction&, const T&>;
			template<detail_::AtomicReadFunction<T> ReadFunction, detail_::AtomicWatchFunction<T> WatchFunction>
				auto
				read
				(ReadFunction&&, WatchFunction&&, std::chrono::steady_clock::duration) const
				-> std::conditional_t
				<
					std::same_as<std::invoke_result_t<ReadFunction&, const T&>, void>,
					bool,
					std::optional<std::invoke_result_t<ReadFunction&, const T&>>
				>;
			template<detail_::AtomicWriteFunction<T> WriteFunction>
				void
				write
				(WriteFunction&&);
			template<detail_::AtomicWriteFunction<T> WriteFunction, detail_::AtomicWatchFunction<T> WatchFunction>
				void
				write
				(WriteFunction&&, WatchFunction&&);
			template<detail_::AtomicWriteFunction<T> WriteFunction, detail_::AtomicWatchFunction<T> WatchFunction>
				bool
				write
				(WriteFunction&&, WatchFunction&&, std::chrono::steady_clock::duration);
			template<detail_::AtomicWatchFunction<T> WatchFunction>
				void
				wait
				(WatchFunction&&) const;
			template<detail_::AtomicWatchFunction<T> WatchFunction>
				bool
				wait
				(WatchFunction&&, std::chrono::steady_clock::duration) const;

		protected:
			mutable
				std::mutex
				Mutex_;
			mutable
				std::condition_variable
				ConditionVariable_;
			T
				Value_;
	};
}
