#include <print>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
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

struct IndirectFixedTypes
{
	std::array<std::uint16_t, 3> samples;
	std::optional<std::uint32_t> maybeCounter;
	std::uint16_t* pointerValue;
	std::span<std::uint16_t, 2> window;
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

	std::uint16_t pointerBacking = 0xCAFE;
	std::array<std::uint16_t, 2> spanBacking{0x0A0B, 0x0C0D};
	IndirectFixedTypes indirectFixed{
		.samples = {0x0102, 0x0304, 0x0506},
		.maybeCounter = 0x11223344U,
		.pointerValue = &pointerBacking,
		.window = std::span<std::uint16_t, 2>{spanBacking},
	};

	std::array<std::uint8_t, leyline::packed_size<IndirectFixedTypes>()> indirectBytes{};
	std::println("IndirectFixedTypes Size: {}, PackedSize: {}", sizeof(IndirectFixedTypes), indirectBytes.size());
	leyline::pack(indirectFixed, indirectBytes);
	std::println("IndirectFixedTypes bytes:");
	print_hex_dump(indirectBytes);

	std::uint16_t unpackedPointerBacking = 0;
	std::array<std::uint16_t, 2> unpackedSpanBacking{};
	IndirectFixedTypes unpackedIndirect{
		.samples = {},
		.maybeCounter = std::nullopt,
		.pointerValue = &unpackedPointerBacking,
		.window = std::span<std::uint16_t, 2>{unpackedSpanBacking},
	};
	leyline::unpack(unpackedIndirect, indirectBytes);

	if (indirectFixed.samples != unpackedIndirect.samples ||
		indirectFixed.maybeCounter != unpackedIndirect.maybeCounter ||
		unpackedIndirect.pointerValue == nullptr ||
		*unpackedIndirect.pointerValue != pointerBacking ||
		spanBacking != unpackedSpanBacking)
	{
		std::println("IndirectFixedTypes roundtrip failed");
		return 1;
	}

	std::array<std::uint16_t, 2> nullSpanBacking{0xDEAD, 0xBEEF};
	IndirectFixedTypes nullableIndirect{
		.samples = {0x1111, 0x2222, 0x3333},
		.maybeCounter = std::nullopt,
		.pointerValue = nullptr,
		.window = std::span<std::uint16_t, 2>{nullSpanBacking},
	};
	std::array<std::uint8_t, leyline::packed_size<IndirectFixedTypes>()> nullableIndirectBytes{};
	leyline::pack(nullableIndirect, nullableIndirectBytes);

	std::uint16_t pointerThatWillBeCleared = 0xFFFF;
	std::array<std::uint16_t, 2> unpackedNullSpanBacking{};
	IndirectFixedTypes unpackedNullableIndirect{
		.samples = {},
		.maybeCounter = 0x1U,
		.pointerValue = &pointerThatWillBeCleared,
		.window = std::span<std::uint16_t, 2>{unpackedNullSpanBacking},
	};
	leyline::unpack(unpackedNullableIndirect, nullableIndirectBytes);

	if (unpackedNullableIndirect.pointerValue != nullptr ||
		unpackedNullableIndirect.maybeCounter.has_value() ||
		nullSpanBacking != unpackedNullSpanBacking)
	{
		std::println("IndirectFixedTypes nullable roundtrip failed");
		return 1;
	}

	std::println("Roundtrip checks passed");
}
