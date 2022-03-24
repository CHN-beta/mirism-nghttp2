# include <mirism.hpp>
using namespace mirism::literals;

struct test_f
{
	constexpr int operator()(int a)
	{
		if (a < 0)
			throw "error";
		return a;
	}
};

int main()
{
	std::optional<int> v1, v2 = 333;
	std::shared_ptr<int> v3, v4 = std::make_shared<int>(333);
	std::weak_ptr<int> v5 = v3, v6 = v4;
	std::unique_ptr<int*> v7 = std::make_unique<int*>(new int(333));
	std::unique_ptr<int> v8 = std::make_unique<int>(333);

	std::cout << "{:x} {:x} {} {} {} {} {}\n"_f(v1, v2, v3, v4, v5, v6, v8);
	std::cout << "{}"_f(v7);

	std::cout << mirism::consteval_invokable<test_f, 1> << mirism::consteval_invokable<test_f, -1> << std::endl;

	std::cout << std::default_initializable<fmt::formatter<int>>;
	std::cout << std::default_initializable<fmt::formatter<int*>>;

	std::cout << mirism::is_static_string<decltype("hello"_ss), char> << mirism::is_static_string<decltype("hello"_ss), char8_t> << std::endl;

	delete *v7;
}