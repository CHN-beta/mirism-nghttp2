# pragma once
# include <mirism/common.hpp>

namespace mirism
{
	namespace detail_
	{
		template <bool Verbose, bool ObjectIndex, bool ThreadIndex>
			class LoggerData
			{};
		template <bool Verbose>
			class LoggerData<Verbose, true, false>
		{
			protected:
				static
					std::map<std::size_t, std::tuple<const void*, std::string_view, std::string>>
					Objects_;
				static
					std::mutex
					ObjectsMutex_;
				static
					std::atomic<std::size_t>
					NextObjectIndex_;
		};
		template <bool Verbose>
			class LoggerData<Verbose, false, true>
		{
			protected:
				static
					std::set<std::size_t>
					Threads_;
				static
					std::mutex
					ThreadsMutex_;
		};
		template <bool Verbose>
			class LoggerData<Verbose, true, true>
			:	public LoggerData<Verbose, true, false>,
				public LoggerData<Verbose, false, true>
			{};

		template <bool Verbose, bool ObjectIndex, bool ThreadIndex>
			class LoggerObjectMonitorData
			{
				protected:
					LoggerObjectMonitorData
						(std::size_t);
			};
		template <bool Verbose, bool ThreadIndex>
			class LoggerObjectMonitorData<Verbose, true, ThreadIndex>
		{
			protected:
				LoggerObjectMonitorData
					(std::size_t);
				const std::size_t
					ObjectIndex_;
		};

		template <bool Verbose, bool ObjectIndex, bool ThreadIndex>
			class LoggerGuardData
			{};
		template <bool ObjectIndex, bool ThreadIndex>
			class LoggerGuardData<true, ObjectIndex, ThreadIndex>
		{
			protected:
				thread_local static
					unsigned Indent_;
				const decltype(std::chrono::system_clock::now())
					StartTime_;
				LoggerGuardData
					(decltype(std::chrono::system_clock::now())&&);
		};

		template
		<
			bool Verbose = detail_::MirismVerbose,
			bool ObjectIndex = detail_::MirismObjectIndex,
			bool ThreadIndex = detail_::MirismObjectIndex
		>
			class Logger
			: public LoggerData<Verbose, ObjectIndex, ThreadIndex>
		{
			public:
				Logger()
				= delete;

				// 发送消息到 tg bot
				static
					void
					notify(const std::string&);
				static
					void
					notify_async(const std::string&);

				// 当回调函数被外部库（不使用该日志系统）调用时，可以将实际执行的代码写为 lambda 表达式传给这个函数，
				// 这个函数会立即执行这个 lambda 表达式，并在执行之前和之后辅以一些操作，以使得线程追踪可以被正常使用。
				// 具体地讲，在开始表达式之前，会检查当前线程是否已经被追踪，如果否，则追踪该线程，
				// 并在 lambda 执行结束并销毁（如果 F 不是左值引用）后停止追踪。
				// 除此以外，还会尝试捕获异常。
				// 所有的模板参数都可以是非引用类型（要求可以移动构造）或者左值引用
				// 对于非引用类型，该函数会负责将其销毁
				template <typename F, typename... Param>
					requires
					(
						std::invocable<F, Param&&...>
						&& (!std::is_rvalue_reference_v<Param> && ...)
						&&
						(
							std::is_lvalue_reference_v<F>
							|| (!std::is_reference_v<F> && std::move_constructible<F>)
						)
					)
					static
					std::invoke_result_t<F, Param&&...>
					exec
					(F&&, Param&&...);

				// 将不带 exec 包装的 lambda 表达式（或其它 invokeable 对象）包装成一个自带 exec 的表达式，
				// 可以方便地制作给外部库使用的回调函数。
				// F 可以是非引用类型或者左值引用，处理方式与 exec 类似：
				// 对于左值引用，返回的对象中只会保留原对象的引用，否则会保留一个移动得到的拷贝。
			protected:
				template <typename F>
					requires (std::is_lvalue_reference_v<F> || (!std::is_reference_v<F> && std::move_constructible<F>))
					class Callback
				{
					public:
						Callback
							(F&&)
							requires std::is_reference_v<F>;
						Callback
							(F&&)
							requires (!std::is_reference_v<F>);
						~Callback
							();
						template <typename... Param>
							requires (std::invocable<F, Param&&...> && (!std::is_rvalue_reference_v<Param> && ...))
							std::invoke_result_t<F, Param&&...>
							operator()
							(Param&&...);

						Callback
							(const Callback&)
							requires std::is_lvalue_reference_v<F>;
						Callback
							(const Callback&)
							requires (!std::is_lvalue_reference_v<F> && std::copy_constructible<F>);
						Callback
							(Callback&&)
							requires std::is_lvalue_reference_v<F>;
						Callback
							(Callback&&)
							requires (!std::is_lvalue_reference_v<F> && std::move_constructible<F>);
						Callback&
							operator=
							(const Callback&)
							= delete;
						Callback&
							operator=
							(Callback&&)
							= delete;

					protected:
						// 对于非引用的情况，需要保留一个指针而不是直接保留对象，以方便监测对象析构的过程
						std::conditional_t<std::is_reference_v<F>, F, std::unique_ptr<F>>
							Function_;
				};
			public:
				template <typename F> requires std::constructible_from<Callback<F>, F&&> static
					Callback<F>
					create_callback
					(F&&);

				class Guard
					: public LoggerGuardData<Verbose, ObjectIndex, ThreadIndex>
				{
					public:
						// 构造一个 Guard 对象以打印日志，可以将函数的传入参数传入构造函数
						// 如果定义了 MIRISM_VERBOSE，则会同时打印离开这个函数的名称，源文件的位置，函数执行的时长等
						template <typename... Param> explicit
							Guard
							(Param&&...)
							requires Verbose;
						template <typename... Param> explicit
							Guard
							(Param&&...)
							requires (!Verbose);
						~Guard
							();

						// 如果定义了 MIRISM_VERBOSE，则会打印此时执行到了源文件的哪里，已经执行了多长时间
						void
							operator()
							() const;

						// 方便地打印函数的返回值，只有定义了 MIRISM_VERBOSE 才会打印
						// 难以检查参数是否可以被 format，暂时不检查
						template <typename T>
							T
							rtn
							(T&&) const;

						// 按照不同的等级打印消息，
						// 若定义了 MIRISM_VERBOSE 则两个函数效果相同，若不定义 MIRISM_VERBOSE 则只有 info 打印信息
						// 打印的内容还包括函数名等，若定义了 MIRISM_VERBOSE 还会打印函数的执行时间等
						// 目前，不严格检查参数是否可以被 format
						void
							debug
							(const std::string&) const;
						void
							info
							(const std::string&) const;

						// 在另一个线程中执行函数并跟踪线程
						template <typename F, typename... Param>
							requires
							(
								std::invocable<F, Param&&...>
								&& !std::is_rvalue_reference_v<F>
								&& (!std::is_rvalue_reference_v<Param> && ...)
							)
							void
							fork
							(F&&, Param&&...) const;
				};

				// 用于追踪一个类的构造和析构
				// 继承该模板（模板参数就是要追踪的类）即可
				// 只有在定义 MIRISM_OBJECT_INDEX 的情况下起作用
				template<typename T>
					class ObjectMonitor
					: public LoggerObjectMonitorData<Verbose, ObjectIndex, ThreadIndex>
				{
					public:
						ObjectMonitor
							(std::string = "");
						~ObjectMonitor
							()
							noexcept(!ObjectIndex);
					protected:
						void append_discription_(const std::string&) const;
				};

			protected:
				static
					std::string
					souce_location_
					(boost::stacktrace::stacktrace::const_reference);
				static
					std::string
					get_thread_id_
					(std::thread::id = std::this_thread::get_id());
		};
	}
	using Logger = detail_::Logger<>;
}
