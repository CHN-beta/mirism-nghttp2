# pragma once
# include <mirism/logger.hpp>

namespace mirism::detail_
{
	template <bool Verbose> inline
		std::map<std::size_t, std::tuple<const void*, std::string_view, std::string>>
		LoggerData<Verbose, true, false>::Objects_;
	template <bool Verbose> inline
		std::mutex
		LoggerData<Verbose, true, false>::ObjectsMutex_;
	template <bool Verbose> inline
		std::atomic<std::size_t>
		LoggerData<Verbose, true, false>::NextObjectIndex_
		= 0;
	template <bool Verbose> inline
		std::set<std::size_t>
		LoggerData<Verbose, false, true>::Threads_;
	template <bool Verbose> inline
		std::mutex
		LoggerData<Verbose, false, true>::ThreadsMutex_;
	template <bool ObjectIndex, bool ThreadIndex> inline thread_local
		unsigned
		LoggerGuardData<true, ObjectIndex, ThreadIndex>::Indent_
		= 0;

	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> inline
		LoggerObjectMonitorData<Verbose, ObjectIndex, ThreadIndex>::LoggerObjectMonitorData
		(std::size_t)
		{}
	template <bool Verbose, bool ThreadIndex> inline
		LoggerObjectMonitorData<Verbose, true, ThreadIndex>::LoggerObjectMonitorData
		(std::size_t index)
		: ObjectIndex_(index)
		{}

	template <bool ObjectIndex, bool ThreadIndex>
		detail_::LoggerGuardData<true, ObjectIndex, ThreadIndex>::LoggerGuardData
		(decltype(std::chrono::system_clock::now())&& time)
		:	StartTime_(std::move(time))
		{}

	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> inline
		void
		Logger<Verbose, ObjectIndex, ThreadIndex>::notify
		(const std::string& message)
	{
		TgBot::Bot bot("xxxxxxxxx");
		bot.getApi().sendMessage(861886506, message);
	}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> inline
		void
		Logger<Verbose, ObjectIndex, ThreadIndex>::notify_async
		(const std::string& message)
		{Logger::Guard().fork([message]{notify(message);});}

	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename F, typename... Param>
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
		inline
		std::invoke_result_t<F, Param&&...>
		Logger<Verbose, ObjectIndex, ThreadIndex>::exec
		(F&& f, Param&&... param)
	{
		if constexpr (ThreadIndex)
		{
			auto& ThreadsMutex_ = LoggerData<Verbose, ObjectIndex, ThreadIndex>::ThreadsMutex_;
			auto& Threads_ = LoggerData<Verbose, ObjectIndex, ThreadIndex>::Threads_;
			bool new_thread = false;
			auto id = std::hash<std::thread::id>()(std::this_thread::get_id());
			Guard log;
			// check new thread
			{
				std::lock_guard lock(ThreadsMutex_);
				if (!Threads_.contains(id))
				{
					log.info("thread 0x{:0{}x} start from exec"_f(id, sizeof(std::size_t) * 2));
					Threads_.insert(id);
					new_thread = true;
				}
				else if constexpr (Verbose)
					log.info("thread 0x{:0{}x} has been tracked"_f(id, sizeof(std::size_t) * 2));
			}
			// exec and wait until finish & destruct
			try
			{
				// if F is lvalue ref, use ref too, else move it
				F f_exec = std::forward<F>(f);
				// 如果 Param&& 是右值引用，那么需要将其移动过来，使它在大括号内被析构
				std::tuple<Param...> params(std::forward<Param>(param)...);
				std::apply(std::forward<F>(f_exec), std::move(params));
			}
			catch (std::exception& ex)
				{log.info(R"(catch exception in exec: "{}")"_f(ex.what()));}
			catch (...)
				{log.info("catch something in exec");}
			// finish
			if (new_thread)
			{
				std::lock_guard lock(ThreadsMutex_);
				if (!Threads_.contains(id))
					log.info("thread 0x{:0{}x} could not found from exec"_f(id, sizeof(std::size_t) * 2));
				else
				{
					Threads_.erase(id);
					log.info("thread 0x{:0{}x} finish from exec"_f(id, sizeof(std::size_t) * 2));
				}
			}
		}
		else
			return std::invoke(std::forward<F>(f), std::forward<Param>(param)...);
	}

	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename F>
		requires (std::is_lvalue_reference_v<F> || (!std::is_reference_v<F> && std::move_constructible<F>)) inline
		Logger<Verbose, ObjectIndex, ThreadIndex>::Callback<F>::Callback
		(F&& function)
		requires std::is_reference_v<F>
		: Function_(function)
		{}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename F>
		requires (std::is_lvalue_reference_v<F> || (!std::is_reference_v<F> && std::move_constructible<F>)) inline
		Logger<Verbose, ObjectIndex, ThreadIndex>::Callback<F>::Callback
		(F&& function)
		requires (!std::is_reference_v<F>)
		: Function_(std::make_unique<F>(std::move(function)))
		{}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename F>
		requires (std::is_lvalue_reference_v<F> || (!std::is_reference_v<F> && std::move_constructible<F>)) inline
		Logger<Verbose, ObjectIndex, ThreadIndex>::Callback<F>::~Callback
		()
	{
		if constexpr (!std::is_lvalue_reference_v<F>)
			Logger::exec([this]{Function_.reset();});
	}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename F>
		requires (std::is_lvalue_reference_v<F> || (!std::is_reference_v<F> && std::move_constructible<F>))
		template <typename... Param>
		requires (std::invocable<F, Param&&...> && (!std::is_rvalue_reference_v<Param> && ...)) inline
		std::invoke_result_t<F, Param&&...>
		Logger<Verbose, ObjectIndex, ThreadIndex>::Callback<F>::operator()
		(Param&&... param)
	{
		if constexpr (std::is_reference_v<F>)
			Logger::exec(Function_, std::forward<Param>(param)...);
		else
		{
			if (Function_)
				Logger::exec(*Function_, std::forward<Param>(param)...);
			else
				Logger::exec([]{Logger::Guard().info("callback function is null");});
		}
	}

	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename F>
		requires (std::is_lvalue_reference_v<F> || (!std::is_reference_v<F> && std::move_constructible<F>)) inline
		Logger<Verbose, ObjectIndex, ThreadIndex>::Callback<F>::Callback
		(const Callback& other)
		requires std::is_lvalue_reference_v<F>
		: Function_(other.Function_)
		{}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename F>
		requires (std::is_lvalue_reference_v<F> || (!std::is_reference_v<F> && std::move_constructible<F>)) inline
		Logger<Verbose, ObjectIndex, ThreadIndex>::Callback<F>::Callback
		(const Callback& other)
		requires (!std::is_lvalue_reference_v<F> && std::copy_constructible<F>)
		: Function_(std::make_unique<F>(*other.Function_))
		{}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename F>
		requires (std::is_lvalue_reference_v<F> || (!std::is_reference_v<F> && std::move_constructible<F>)) inline
		Logger<Verbose, ObjectIndex, ThreadIndex>::Callback<F>::Callback
		(Callback&& other)
		requires std::is_lvalue_reference_v<F>
		: Function_(other.Function_)
		{}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename F>
		requires (std::is_lvalue_reference_v<F> || (!std::is_reference_v<F> && std::move_constructible<F>)) inline
		Logger<Verbose, ObjectIndex, ThreadIndex>::Callback<F>::Callback
		(Callback&& other)
		requires (!std::is_lvalue_reference_v<F> && std::move_constructible<F>)
		: Function_(std::move(other.Function_))
		{}

	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename F>
		requires std::constructible_from<typename Logger<Verbose, ObjectIndex, ThreadIndex>::Callback<F>, F&&> inline
		auto
		Logger<Verbose, ObjectIndex, ThreadIndex>::create_callback
		(F&& f)
		-> Callback<F>
		{return Callback<F>(std::forward<F>(f));}

	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename... Param> inline
		Logger<Verbose, ObjectIndex, ThreadIndex>::Guard::Guard
		(Param&&... param)
		requires (Verbose)
		: LoggerGuardData<Verbose, ObjectIndex, ThreadIndex>({std::chrono::system_clock::now()})
	{
		auto& Indent_ = LoggerGuardData<Verbose, ObjectIndex, ThreadIndex>::Indent_;
		std::osyncstream stream(std::clog);
		boost::stacktrace::stacktrace stack;
		stream << "{}|>  [ {} {} ] begin {}"_f
		(
			std::string(Indent_ * 4, ' '),
			get_thread_id_(),
			souce_location_(stack[1]),
			stack[1].name()
		);
		if constexpr (sizeof...(param) > 0)
		{
			stream << " with (";
			std::vector<std::string> args = {"{}"_f(param)...};
			for (std::size_t i = 0; i < args.size(); i++)
			{
				stream << args[i];
				if (i < args.size() - 1)
					stream << ", ";
			}
			stream << ")\n";
		}
		else
			stream << "\n";
		Indent_++;
	}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename... Param> inline
		Logger<Verbose, ObjectIndex, ThreadIndex>::Guard::Guard
		(Param&&...)
		requires (!Verbose)
		{}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> inline
		Logger<Verbose, ObjectIndex, ThreadIndex>::Guard::~Guard
		()
	{
		if constexpr (Verbose)
		{
			auto& Indent_ = LoggerGuardData<Verbose, ObjectIndex, ThreadIndex>::Indent_;
			auto& StartTime_ = LoggerGuardData<Verbose, ObjectIndex, ThreadIndex>::StartTime_;
			std::osyncstream stream(std::clog);
			boost::stacktrace::stacktrace stack;
			stream << "{}|>  [ {} {} ] finish {} after {}ms\n"_f
			(
				std::string((Indent_ - 1) * 4, ' '),
				get_thread_id_(),
				souce_location_(stack[1]),
				stack[1].name(),
				(std::chrono::system_clock::now() - StartTime_).count() / 1e6
			);
			Indent_--;
		}
	}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> inline
		void Logger<Verbose, ObjectIndex, ThreadIndex>::Guard::operator()
		() const
	{
		if constexpr (Verbose)
		{
			auto& Indent_ = LoggerGuardData<Verbose, ObjectIndex, ThreadIndex>::Indent_;
			auto& StartTime_ = LoggerGuardData<Verbose, ObjectIndex, ThreadIndex>::StartTime_;
			std::osyncstream stream(std::clog);
			boost::stacktrace::stacktrace stack;
			stream << "{}|>  [ {} {} after {}ms]\n"_f
			(
				std::string(Indent_ * 4, ' '),
				get_thread_id_(),
				souce_location_(stack[1]),
				(std::chrono::system_clock::now() - StartTime_).count() / 1e6
			);
		}
	}

	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename T> inline
		T
		Logger<Verbose, ObjectIndex, ThreadIndex>::Guard::rtn
		(T&& r) const
	{
		if constexpr (Verbose)
		{
			auto& Indent_ = LoggerGuardData<Verbose, ObjectIndex, ThreadIndex>::Indent_;
			std::osyncstream stream(std::clog);
			boost::stacktrace::stacktrace stack;
			stream << "{}|>  [ {} {} ] return {} {}"_f
			(
				std::string((Indent_ - 1) * 4, ' '),
				get_thread_id_(),
				souce_location_(stack[1]),
				nameof::nameof_full_type<T>(),
				r
			);
		}
		return std::forward<T>(r);
	}

	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> [[gnu::always_inline]] inline
		void
		Logger<Verbose, ObjectIndex, ThreadIndex>::Guard::debug
		(const std::string& msg) const
	{
		if constexpr (Verbose)
		{
			auto& Indent_ = LoggerGuardData<Verbose, ObjectIndex, ThreadIndex>::Indent_;
			std::osyncstream stream(std::clog);
			boost::stacktrace::stacktrace stack;
			stream << "{}|>  [ {} {} ] {}\n"_f
			(
				std::string(Indent_ * 4, ' '),
				get_thread_id_(),
				souce_location_(stack[1]),
				msg
			);
		}
	}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> [[gnu::always_inline]] inline
		void
		Logger<Verbose, ObjectIndex, ThreadIndex>::Guard::info
		(const std::string& msg) const
	{
		if constexpr (Verbose)
			debug(msg);
		else
			std::osyncstream(std::clog) << "[{}] {}\n"_f(std::chrono::system_clock::now(), msg);
	}

	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename F, typename... Param>
		requires
		(
			std::invocable<F, Param&&...>
			&& !std::is_rvalue_reference_v<F>
			&& (!std::is_rvalue_reference_v<Param> && ...)
		)
		void
		Logger<Verbose, ObjectIndex, ThreadIndex>::Guard::fork
		(F&& f, Param&&... param) const
	{
		// 需要将所有右值引用的参数都移动到 lambda 中
		std::jthread new_thread
		(
			[f_params = std::tuple<F, Param...>(std::forward<F>(f), std::forward<Param>(param)...)] mutable
				{std::apply(Logger::exec<F, Param...>, std::move(f_params));}
		);
		if constexpr (ThreadIndex)
		{
			if (auto id = new_thread.get_id(); id != std::thread::id())
				debug("fork thread {} from {}"_f(get_thread_id_(id), get_thread_id_()));
			else
				debug("fork thread {} from {}"_f("(unknown)", get_thread_id_()));
		}
		new_thread.detach();
	}

	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename T> inline
		Logger<Verbose, ObjectIndex, ThreadIndex>::ObjectMonitor<T>::ObjectMonitor
		(std::string desc)
		:	LoggerObjectMonitorData<Verbose, ObjectIndex, ThreadIndex>
			(
				[]
				{
					if constexpr (ObjectIndex)
						return Logger<Verbose, ObjectIndex, ThreadIndex>::NextObjectIndex_++;
					else
						return 0;
				}
				()
			)
	{
		if constexpr (ObjectIndex)
		{
			Logger::Guard log(fmt::ptr(this), desc);
			auto p = static_cast<const T*>(this);
			auto& Objects_ = LoggerData<Verbose, ObjectIndex, ThreadIndex>::Objects_;
			auto& ObjectIndex_ = LoggerObjectMonitorData<Verbose, ObjectIndex, ThreadIndex>::ObjectIndex_;
			log.info("create object {} {} {} {}"_f(ObjectIndex_, fmt::ptr(p), nameof::nameof_type<T>(), desc));
			std::lock_guard lock(LoggerData<Verbose, ObjectIndex, ThreadIndex>::ObjectsMutex_);
			if (Objects_.find(ObjectIndex_) != Objects_.end()) [[unlikely]]
				throw std::runtime_error("object {} already exists"_f(ObjectIndex_));
			else
				Objects_.emplace(ObjectIndex_, std::tuple{p, nameof::nameof_type<T>(), std::move(desc)});
		}
	}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename T> inline
		Logger<Verbose, ObjectIndex, ThreadIndex>::ObjectMonitor<T>::~ObjectMonitor
		()
		noexcept(!ObjectIndex)
	{
		if constexpr (ObjectIndex)
		{
			Logger::Guard log(fmt::ptr(this));
			auto& Objects_ = LoggerData<Verbose, ObjectIndex, ThreadIndex>::Objects_;
			auto& ObjectIndex_ = LoggerObjectMonitorData<Verbose, ObjectIndex, ThreadIndex>::ObjectIndex_;
			std::lock_guard lock(LoggerData<Verbose, ObjectIndex, ThreadIndex>::ObjectsMutex_);
			if (Objects_.find(ObjectIndex_) == Objects_.end()) [[unlikely]]
				throw std::runtime_error("object {} not exists"_f(ObjectIndex_));
			else
			{
				log.info("destroy object {} {}"_f(ObjectIndex_, Objects_[ObjectIndex_]));
				Objects_.erase(ObjectIndex_);
			}
		}
	}
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> template <typename T> inline
		void
		Logger<Verbose, ObjectIndex, ThreadIndex>::ObjectMonitor<T>::append_discription_
		(const std::string& desc) const
	{
		if constexpr (ObjectIndex)
		{
			Logger::Guard log(fmt::ptr(this), desc);
			auto& Objects_ = LoggerData<Verbose, ObjectIndex, ThreadIndex>::Objects_;
			auto& ObjectIndex_ = LoggerObjectMonitorData<Verbose, ObjectIndex, ThreadIndex>::ObjectIndex_;
			std::lock_guard lock(LoggerData<Verbose, ObjectIndex, ThreadIndex>::ObjectsMutex_);
			if (Objects_.find(ObjectIndex_) == Objects_.end()) [[unlikely]]
				throw std::runtime_error("object {} not exists"_f(ObjectIndex_));
			else
			{
				std::get<2>(Objects_[ObjectIndex_]) += desc;
				log.info("modify object description {} {}"_f(ObjectIndex_, Objects_[ObjectIndex_]));
			}
		}
	}

	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> inline
		std::string
		Logger<Verbose, ObjectIndex, ThreadIndex>::souce_location_
		(boost::stacktrace::stacktrace::const_reference source)
	{
		if (std::string file = source.source_file(); file == "")
			return "??:??";
		else
		{
			if (auto floc = file.rfind("mirism/"); floc != std::string::npos)
				file = file.substr(floc + std::string("mirism/").length());
			return "{}:{}"_f(file, source.source_line());
		}
	};
	template <bool Verbose, bool ObjectIndex, bool ThreadIndex> inline
		std::string
		Logger<Verbose, ObjectIndex, ThreadIndex>::get_thread_id_
		(std::thread::id id)
		{return "{:04x}"_f(std::hash<std::thread::id>()(id) % std::numeric_limits<std::uint16_t>::max());};
}
