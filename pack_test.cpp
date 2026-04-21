#include <print>
#include <array>
#include <cstdint>
#include <utility>
#include "leyline.hpp"
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

struct [[=leyline::wire::big]] PacketHeader
{
	std::uint16_t messageType;
	[[=leyline::wire::little]] std::uint16_t revision;

	bool operator==(const PacketHeader&) const noexcept = default;
};

struct NetworkPacket
{
	std::uint16_t preamble;
	[[=leyline::wire::big]] std::uint16_t payloadLength;
	PacketHeader header;
	std::uint32_t checksum;

	bool operator==(const NetworkPacket&) const noexcept = default;
};

int main()
{
	TestStruct testStruct{true, 42, 3.5};
	std::array<std::uint8_t, leyline::packed_size<TestStruct>()> testStructBytes{};
	std::println("TestStruct Size: {}, PackedSize: {}", sizeof(TestStruct), testStructBytes.size());
	leyline::pack(testStruct, testStructBytes);
	std::println("TestStruct bytes:");
	print_hex_dump(testStructBytes);
	const auto unpackedTestStruct = leyline::unpack<TestStruct>(testStructBytes);
	if (testStruct != unpackedTestStruct)
	{
		std::println("TestStruct roundtrip failed");
		return 1;
	}

	Nested nested{{7}, {11}, {1.25F, 2.5F}};
	std::array<std::uint8_t, leyline::packed_size<Nested>()> nestedBytes{};
	std::println("Nested Size: {}, PackedSize: {}", sizeof(Nested), nestedBytes.size());
	leyline::pack(nested, nestedBytes);
	std::println("Nested bytes:");
	print_hex_dump(nestedBytes);
	const auto unpackedNested = leyline::unpack<Nested>(nestedBytes);
	if (nested != unpackedNested)
	{
		std::println("Nested roundtrip failed");
		return 1;
	}

	NetworkPacket packet{
		.preamble = 0x1234,
		.payloadLength = 0xABCD,
		.header = {.messageType = 0x3456, .revision = 0x789A},
		.checksum = 0x01020304,
	};
	std::array<std::uint8_t, leyline::packed_size<NetworkPacket>()> packetBytes{};
	constexpr std::array<std::uint8_t, leyline::packed_size<NetworkPacket>()> expectedPacketBytes{
		0x34,
		0x12,
		0xAB,
		0xCD,
		0x34,
		0x56,
		0x9A,
		0x78,
		0x04,
		0x03,
		0x02,
		0x01,
	};

	std::println("NetworkPacket Size: {}, PackedSize: {}", sizeof(NetworkPacket), packetBytes.size());
	leyline::pack(packet, packetBytes);
	std::println("NetworkPacket bytes:");
	print_hex_dump(packetBytes);

	if (packetBytes != expectedPacketBytes)
	{
		std::println("NetworkPacket byte order check failed");
		return 1;
	}

	const auto unpackedPacket = leyline::unpack<NetworkPacket>(packetBytes);
	if (packet != unpackedPacket)
	{
		std::println("NetworkPacket roundtrip failed");
		return 1;
	}

	std::println("Roundtrip checks passed");
}
