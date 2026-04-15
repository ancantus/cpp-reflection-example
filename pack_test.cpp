#include <print>
#include <array>
#include <cstdint>
#include <utility>
#include "pack.hpp"
#include "print.hpp"

struct TestStruct
{
	bool one;
	int two;
	double three;
};

struct NestedStruct1
{
	short int one;
};

struct NestedStruct2
{
	int one;
};

struct Nested
{
	NestedStruct1 one;
	NestedStruct2 two;
	std::pair<float, float> three;
};

int main()
{
	std::println("TestStruct Size: {}, PackedSize: {}", sizeof(TestStruct), packed_size<TestStruct>());
	std::println("Nested Size: {}, PackedSize: {}", sizeof(Nested), packed_size<Nested>());

	TestStruct testStruct{true, 42, 3.5};
	std::array<uint8_t, packed_size<TestStruct>()> testStructBytes{};
	pack(testStruct, testStructBytes);
	std::println("TestStruct bytes:");
	print_hex_dump(testStructBytes);

	Nested nested{{7}, {11}, {1.25F, 2.5F}};
	std::array<uint8_t, packed_size<Nested>()> nestedBytes{};
	pack(nested, nestedBytes);
	std::println("Nested bytes:");
	print_hex_dump(nestedBytes);
}
