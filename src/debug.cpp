# include <mirism.hpp>
using namespace mirism::literals;

int main()
{
	mirism::Server
	<
		mirism::server::Mirror
		<
			mirism::server::mirror::site::Wikipedia,
			mirism::server::mirror::site::google::Scholar,
			mirism::server::mirror::site::google::Search,
			mirism::server::mirror::site::google::Patents,
			mirism::server::mirror::site::scihub::Ru,
			mirism::server::mirror::site::scihub::Se,
			mirism::server::mirror::site::google::Youtube,
			mirism::server::mirror::site::hcaptcha::Server,
			mirism::server::mirror::site::Github
		>,
		mirism::server::Api<mirism::server::api::Compile>
	>({}, {})("debug.mirism.one", "443");
}
