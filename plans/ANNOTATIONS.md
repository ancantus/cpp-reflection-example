# Annotation Notes

This file summarizes the annotation design discussion for this reflection-based packing prototype.

## What reflection can see

- Use C++26-style annotations: `[[=expr]]`.
- Annotation values are compile-time values (structural types are a good default).
- Reflection can query them with `annotations_of(...)` / `annotations_of_with_type(...)` and decode with `extract<T>(...)`.
- `//` and `/* ... */` comments are not reflectable metadata.

## Endianness API shape

Example annotation payload types:

```cpp
namespace wire {
  enum class byte_order : std::uint8_t { little, big };

  struct endian {
    byte_order value;
    constexpr bool operator==(const endian&) const = default;
  };

  inline constexpr endian little{byte_order::little};
  inline constexpr endian big{byte_order::big};
}
```

Example usage on types (including nested sub-structs with different policy):

```cpp
struct [[=wire::big]] NetworkHeader {
  std::uint16_t msg_type;
  std::uint32_t sequence;
};

struct [[=wire::little]] LegacyPayload {
  std::uint16_t sample_count;
  std::uint32_t checksum;
};

struct [[=wire::big]] Envelope {
  std::uint16_t magic;
  NetworkHeader net;
  LegacyPayload payload;
};
```

Example field override:

```cpp
struct [[=wire::big]] MixedRecord {
  std::uint32_t id;
  [[=wire::little]] std::uint32_t ts;
  std::uint16_t flags;
};
```

Recommended precedence rule:

1. Field annotation (highest)
2. Field type annotation
3. Enclosing struct/class annotation
4. Library default

## Checksum and length-prefix extensibility

Goal: library supplies framework; users supply algorithms/policies.

Pattern:

- Annotation carries tag/config (`checksum_field<Tag>`, `checksum_region<Tag>`, `length_prefix<Tag>`).
- Library resolves annotation via reflection.
- Implementation dispatches to user-provided traits/CPO (`checksum_traits<Tag>`, `length_prefix_traits<Tag>`).

Sketch:

```cpp
namespace wire {
  template <class AlgoTag>
  struct checksum_field {
    std::uint16_t region_id = 0;
    constexpr bool operator==(const checksum_field&) const = default;
  };

  template <class AlgoTag>
  struct checksum_region {
    std::uint16_t region_id = 0;
    constexpr bool operator==(const checksum_region&) const = default;
  };

  template <class LenTag>
  struct length_prefix {
    constexpr bool operator==(const length_prefix&) const = default;
  };
}
```

This enables built-in algorithms (for example CRC32) and user algorithms without changing core pack logic.

## Built-in shorthand names vs generic tags

Desired shorthand:

```cpp
[[=wire::checksum_field_crc32{.region_id = 1}]]
```

Canonical generic form:

```cpp
[[=wire::checksum_field<crc32_tag>{.region_id = 1}]]
```

Important constraint:

- There is no automatic "parse suffix `_crc32` and instantiate `<crc32_tag>`" mechanism in reflection.
- For built-in ergonomic names, define explicit alias/wrapper annotation types and map them with traits/specializations.
- Keep generic tag-based annotations for user-defined algorithms.
