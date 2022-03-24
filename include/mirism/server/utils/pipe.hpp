# pragma once
# include <mirism/logger.hpp>
# include <mirism/server/utils/atomic.hpp>

namespace mirism::server::utils
{
	// 用于两个线程之间传递字符串（请求体和响应体）
	// 需要保证 pipe 是通过 std::shared_ptr 来保存的
	// 注意，与 Atomic 一致，仍然在这里忽略占用锁的时间
	class Pipe
		:	public std::enable_shared_from_this<Pipe>,
			public Logger::ObjectMonitor<Pipe>
	{
		public:
			Pipe
			();

			// 将数据写入 pipe，并等待直到数据被 read 函数读取完毕，然后关闭 pipe
			// 第二个参数指定最长时间，在等待数据可以被写入、等待数据被读取完毕这两个过程中分别起作用，
			//	  不会因为上一个阶段消耗了一些时间而导致下一个阶段的等待时间变短
			// 若写入一切顺利，则返回 true，任何一步失败都会返回 false（在网络情况不佳等意外情况下，等待写入、等待读取完毕可能会超时）
			// pipe 可能会被其它的线程关闭，这时视为成功
			bool
			write
			(const std::string_view&, std::chrono::steady_clock::duration);

			// 返回一个回调函数，直接将该函数交给 nghttp2::xxxx::on_data 以将数据写入 pipe，返回值中会包含一个此对象的 shared_ptr
			// 如果使用该回调函数写入一段数据（长度非零），回调函数会等待至可以写入的状态，然后写入并等待被读取完毕
			// 与其它函数一样，参数决定每步的等待时间
			// 如果使用该回调函数写入一段空数据，回调函数会等待至可以写入的状态，然后关闭 pipe，等待的时长也由此函数的参数设定
			nghttp2::asio_http2::data_cb
			write
			(std::chrono::steady_clock::duration);

			// 从 pipe 中读取所有数据到一个字符串，也即一直读取直到 Pipe 被关闭，然后返回一个字符串
			// 第二个参数指定每一次等待容许的最长时间，超过这个时间后即认为 pipe 已经“损坏”，将 pipe 关闭并返回 std::nullopt
			// 如果一切顺利，返回读取到的字符串，否则返回 std::nullopt
			std::optional<std::string>
			read_all
			(std::chrono::steady_clock::duration);

			// 返回一个回调函数，直接将该函数交给 nghttp2::xxxx::on_data 以从 pipe 中读入数据，返回值中会包含一个此对象的 shared_ptr
			// 传入参数指定每次最长等待的时间
			nghttp2::asio_http2::generator_cb
			read
			(std::chrono::steady_clock::duration);

			// 检查 pipe 是否已经被关闭，执行需要的时间可以被忽略
			bool
			end
			() const;

			// 等待直到 pipe 被关闭或者被写入了内容，传入最长等待时间
			// 如果被关闭，则返回 true，否则返回 false，如果超时，则返回 std::nullopt
			bool
			empty
			() const;
			std::optional<bool>
			empty
			(std::chrono::steady_clock::duration) const;

			// 关闭 pipe，执行需要的时间可以被忽略，并且一定会关闭成功
			void
			shutdown
			();
		protected:

			// Pipe 应该有三种状态：
			//  * 已经被关闭
			//	  这时 End 为 true，ReadyToRead 为 true 或者 false，Data 为空
			//	  这时无论读取还是写入，都应该立即返回
			//  * 准备好被写入
			//	  这时 End 为 false，ReadyToRead 为 false，Data 为空
			//	  若这时尝试读取，则应该等待直到进入其它两种状态（准备好被读取，或已经被关闭）
			//	  若这时尝试写入非空的数据，则应该写入 Data 并将置为准备好被读取，通知其它线程读取
			//	  然后等待直到 Pipe 回到准备好被写入的状态或者被关闭
			//	  若这时尝试写入空数据，则应该将 Pipe 关闭
			//  * 准备好被读取
			//	  这时 End 为 false，ReadyToRead 为 true，Data 不为空并且长度不为零
			//	  若这时尝试读取，则应该读取需要的长度并调用 Data 的 remove_prefix；
			//	  若 Data 长度减为零，则将 Pipe 置于准备好被写入然后返回；否则，不修改 Pipe 的状态直接返回
			//	  若这时尝试写入，则应该等待直到进入其它两种状态（准备好被写入，或已经被关闭）
			// 上述的“等待”都是有时间限制的。若超过了时间限制，则视为 Pipe 已经损坏，关闭 Pipe 并返回。
			// 按照“忽略占用锁的时间”的约定，Pipe 应该是随时可以被关闭的，并且关闭所花费的时间也是可以忽略不计的
			struct DataType_
			{
				bool
					End;
				bool
					ReadyToRead;
				std::string_view
					Data;
			};
			Atomic<DataType_>
				Data_;
	};
}
