#include <meta>
#include <print>
typedef enum { RED } Color;

Color c;
static_assert(identifier_of(^^decltype(c)) == "Color1");

int main()
{
	std::println("hello world!");
}
