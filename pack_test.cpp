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

	bool operator==(const TestStruct&) const noexcept = default;
};

struct NestedStruct1
{
	short int one;

	bool operator==(const NestedStruct1&) const noexcept = default;
};

struct NestedStruct2
{
	int one;

	bool operator==(const NestedStruct2&) const noexcept = default;
};

struct Nested
{
	NestedStruct1 one;
	NestedStruct2 two;
	std::pair<float, float> three;

	bool operator==(const Nested&) const noexcept = default;
};

int main()
{
	TestStruct testStruct{true, 42, 3.5};
	std::array<uint8_t, packed_size<TestStruct>()> testStructBytes{};
	std::println("TestStruct Size: {}, PackedSize: {}", sizeof(TestStruct), testStructBytes.size());
	pack(testStruct, testStructBytes);
	std::println("TestStruct bytes:");
	print_hex_dump(testStructBytes);
	const auto unpackedTestStruct = unpack<TestStruct>(testStructBytes);
	if (testStruct != unpackedTestStruct)
	{
		std::println("TestStruct roundtrip failed");
		return 1;
	}

	Nested nested{{7}, {11}, {1.25F, 2.5F}};
	std::array<uint8_t, packed_size<Nested>()> nestedBytes{};
	std::println("Nested Size: {}, PackedSize: {}", sizeof(Nested), nestedBytes.size());
	pack(nested, nestedBytes);
	std::println("Nested bytes:");
	print_hex_dump(nestedBytes);
	const auto unpackedNested = unpack<Nested>(nestedBytes);
	if (nested != unpackedNested)
	{
		std::println("Nested roundtrip failed");
		return 1;
	}

	std::println("Roundtrip checks passed");
}
