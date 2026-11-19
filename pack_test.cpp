#include <print>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include "leyline.hpp"

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

struct CrcPayload
{
	std::uint16_t first;
	std::uint16_t second;
};

struct CrcMessageDefault
{
	std::uint16_t magic;
	[[=leyline::checksum_field_crc32{}]] std::uint32_t crc;
	[[=leyline::checksum_region{}]] CrcPayload payload;
};

struct payload_region_tag
{
};

struct CrcMessageTagged
{
	std::uint16_t magic;
	[[=leyline::checksum_field_crc32<payload_region_tag>{}]] std::uint32_t crc;
	[[=leyline::checksum_region<payload_region_tag>{}]] CrcPayload payload;
};

struct CrcMessageById
{
	std::uint16_t magic;
	[[=leyline::checksum_field_crc32{.region_id = 7}]] std::uint32_t crc;
	[[=leyline::checksum_region{.region_id = 7}]] CrcPayload payload;
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
	std::println("IndirectFixedTypes nullable bytes:");
	print_hex_dump(nullableIndirectBytes);

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

	constexpr std::uint32_t expectedCrc32 = 0x1D5E9AF9U;
	CrcPayload crcPayload{.first = 0x1122, .second = 0x3344};

	CrcMessageDefault crcDefault{
		.magic = 0x1234,
		.crc = 0x0,
		.payload = crcPayload,
	};
	std::array<std::uint8_t, leyline::packed_size<CrcMessageDefault>()> crcDefaultBytes{};
	constexpr std::array<std::uint8_t, leyline::packed_size<CrcMessageDefault>()> expectedCrcDefaultBytes{
		0x34,
		0x12,
		0xF9,
		0x9A,
		0x5E,
		0x1D,
		0x22,
		0x11,
		0x44,
		0x33,
	};
	leyline::pack(crcDefault, crcDefaultBytes);
	std::println("CrcMessageDefault bytes:");
	print_hex_dump(crcDefaultBytes);
	if (crcDefaultBytes != expectedCrcDefaultBytes)
	{
		std::println("CRC default region packing failed");
		return 1;
	}
	const auto unpackedCrcDefault = leyline::unpack<CrcMessageDefault>(crcDefaultBytes);
	if (unpackedCrcDefault.magic != crcDefault.magic ||
		unpackedCrcDefault.crc != expectedCrc32 ||
		unpackedCrcDefault.payload.first != crcPayload.first ||
		unpackedCrcDefault.payload.second != crcPayload.second)
	{
		std::println("CRC default region roundtrip failed");
		return 1;
	}

	CrcMessageTagged crcTagged{
		.magic = 0x5566,
		.crc = 0x0,
		.payload = crcPayload,
	};
	std::array<std::uint8_t, leyline::packed_size<CrcMessageTagged>()> crcTaggedBytes{};
	constexpr std::array<std::uint8_t, leyline::packed_size<CrcMessageTagged>()> expectedCrcTaggedBytes{
		0x66,
		0x55,
		0xF9,
		0x9A,
		0x5E,
		0x1D,
		0x22,
		0x11,
		0x44,
		0x33,
	};
	leyline::pack(crcTagged, crcTaggedBytes);
	std::println("CrcMessageTagged bytes:");
	print_hex_dump(crcTaggedBytes);
	if (crcTaggedBytes != expectedCrcTaggedBytes)
	{
		std::println("CRC tagged region packing failed");
		return 1;
	}
	const auto unpackedCrcTagged = leyline::unpack<CrcMessageTagged>(crcTaggedBytes);
	if (unpackedCrcTagged.magic != crcTagged.magic ||
		unpackedCrcTagged.crc != expectedCrc32 ||
		unpackedCrcTagged.payload.first != crcPayload.first ||
		unpackedCrcTagged.payload.second != crcPayload.second)
	{
		std::println("CRC tagged region roundtrip failed");
		return 1;
	}

	CrcMessageById crcById{
		.magic = 0x7788,
		.crc = 0x0,
		.payload = crcPayload,
	};
	std::array<std::uint8_t, leyline::packed_size<CrcMessageById>()> crcByIdBytes{};
	constexpr std::array<std::uint8_t, leyline::packed_size<CrcMessageById>()> expectedCrcByIdBytes{
		0x88,
		0x77,
		0xF9,
		0x9A,
		0x5E,
		0x1D,
		0x22,
		0x11,
		0x44,
		0x33,
	};
	leyline::pack(crcById, crcByIdBytes);
	std::println("CrcMessageById bytes:");
	print_hex_dump(crcByIdBytes);
	if (crcByIdBytes != expectedCrcByIdBytes)
	{
		std::println("CRC region_id packing failed");
		return 1;
	}
	const auto unpackedCrcById = leyline::unpack<CrcMessageById>(crcByIdBytes);
	if (unpackedCrcById.magic != crcById.magic ||
		unpackedCrcById.crc != expectedCrc32 ||
		unpackedCrcById.payload.first != crcPayload.first ||
		unpackedCrcById.payload.second != crcPayload.second)
	{
		std::println("CRC region_id roundtrip failed");
		return 1;
	}

	std::println("Roundtrip checks passed");
}
