# include <mirism.hpp>

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
			mirism::server::mirror::site::hcaptcha::Server
		>,
		mirism::server::Api<mirism::server::api::Compile>
	>
	(
		{},
		{mirism::server::mirror::patch::origin_restrict({"mirism.one"}, {"216.24.178.192"}, "https://mirism.one"), {}}
	)
		("ng01.mirism.one", "7411");
}
