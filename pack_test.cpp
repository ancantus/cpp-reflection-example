#include <print>
#include "pack.hpp"

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
}
