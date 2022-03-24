# pragma once
# include <mirism/server/base.hpp>

namespace mirism
{
	template<std::derived_from<server::Base<>>... Ts>
		class Server
		: public Logger::ObjectMonitor<Server<Ts...>>
	/*
		Main class of libmirism.
		This class is responsible to listen on the specified port and handing off the received requests to the actual
			server object (specified by template parameters, commonly is instance of `mirism::server::Mirror`) for
			processing.
		Just create an instance of this class and call `operator()` to start the server.
		The call of `operator()` will block until the server is stopped.

		For example:
			mirism::Server<mirism::server::Mirror<MySite>>({}, {})("debug.mirism.one", "443");

		Template parameters:
			* List of ervers you want to use. `mirism::server::Miror` is commonly used.
	*/
	{
		public:

			/*
				Callback function to change the behavior of the instance of `Server`.
				The callback function will be set corresponding to a `PatchTiming` value, which will be described
					below.

				Input parameters:
					* The hostname of the server, e.g., debug.mirism.one, not the hostname be mirrored.
					  The hostname is determined by the parameter of `mirism::Server::operator()`, instead of read from
					  	the request.
					* The actual server object (instance of any class inherited from `mirism::server::Base<>`) that is
						responsible to handling the request.
					* The request object from nghttp2 library.
					* The response object from nghttp2 library.

				Output parameters:
					* Whether to continue the request handling or not.
					  If `false` is returned, the request handling will be stopped, all the following callbacks
					  (including the invoke of `mirism::server::Base<>::operator()` if it is not invoked) will be
					  ignored.
					  Otherwise, the request handling will continue.
			*/
			using patch_t
				= std::function
				<
					bool
					(
						const std::string&, server::Base<>&,
						const nghttp2::asio_http2::server::request&, const nghttp2::asio_http2::server::response&
					)
				>;

			/*
				The timing of the invoke of the corresponding `patch_t`: before the invoke of
					`mirism::server::Base<>::operator()` (`BeforeHandle`) or after it (`AfterHandle`).
			*/
			enum class PatchTiming
			{
				BeforeHandle,
				AfterHandle
			};

			/*
				Constructor.
				Any needed server (instance of any class inherited from `mirism::server::Base<>`) will be created in
					this constructor.
				However, the server object will not going to listen on the port until the `operator()` is called.

				Input parameters:
					* A list of `patch_t` (and corresponding `PatchTimging`) which will be applied to this object.
					  The patch will be called before or after the invoke of `mirism::server::Base<>::operator()`,
					  	depending on the `PatchTiming` value.
					  The order of applications of `patch_t`s with same `PatchTiming` is not garanted.
					  If an `patch_t` returns `false`, the request handling will be stopped, as described above.
					* A tuple consists of all patches which will be applied to each server object respectivly.
					  The patch will be moved to the constructor of each server object.
					  There is a plan to expand this parameter into something like `std::tuple<std::tuple<...>>`, which
					  	allows to pass more than one argument to the constructor of each server object, and
						automatically match propriate constructor according to the arguments passed in.
			*/
			Server
				(std::unordered_multimap<PatchTiming, patch_t>, std::tuple<typename Ts::patch_t...>);

			/*
				Actually run the server.
				This function will block until the internal server
					(nghttp2::asio_http2::server::http2::listen_and_serve) is stopped.

				Input parameters:
					* Hostname of the server (e.g., debug.mirism.one).
					  This variable will be use in following manners:
						* Print log, as well as send message to telegram.
						* Read certificate and key from certbot's default path:
							"/etc/letsencrypt/live/{hostname}fullchain.pem" for certificate and
							"/etc/letsencrypt/live/{hostname}/privkey.pem" for key.
						* Pass to each server object and patches.
					  The server will always listen on "localhost", whatever hostname is specified. Also, the hostname
					  	of received request will never be checked.
					* Port to listen. Will only be used in nghttp2::asio_http2::server::http2::listen_and_serve and
						print some log.
			*/
			void
				operator()
				(const std::string&, const std::string&);

		protected:
			std::unordered_multimap<PatchTiming, patch_t>
				PatchMap_;
			const std::unordered_map<std::string_view, std::shared_ptr<server::Base<>>>
				ServerMap_;
	};
}
