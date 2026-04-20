# C++26 Generic Byte Writer: Design & Trade-offs

This document summarizes the architectural decisions and trade-offs for a generic reflection-based byte serializer targeting C++26.

## 1. Core Architecture: Push vs. Pull

### Push Model (`output_range`) - **Recommended Core API**
- **Interface:** `void pack_to(const T& input, std::ranges::output_range<uint8_t> auto&& output)`
- **Performance:** Highest. Allows the compiler to inline writing logic and potentially vectorize (SIMD).
- **Flexibility:** Works natively with `std::vector` (via `back_inserter`), `std::span`, and `std::array`.
- **Memory:** Zero-allocation; uses caller-provided memory.

### Pull Model (`std::generator`)
- **Interface:** `std::generator<uint8_t> pack(const T& input)`
- **Performance:** Lower due to coroutine overhead (heap allocation for frame, indirect jumps, state machine).
- **Flexibility:** Highest composition with `std::ranges` (e.g., `pack(s) | std::views::filter(...)`).
- **Use Case:** Best for complex, recursive data structures where performance is secondary to readability.

---

## 2. The "Return" Interface (Fixed-Size Types)

For types where the serialized size is known at compile-time, a "Return by Value" interface is provided.

### `std::array` Return
- **Constraint:** `requires (is_fixed_size<T>())`
- **Implementation:**
  ```cpp
  template <typename T>
  constexpr auto pack(const T& input) -> std::array<uint8_t, serialized_size<T>()> {
      std::array<uint8_t, serialized_size<T>()> buffer;
      pack_to(input, buffer);
      return buffer;
  }
  ```
- **Benefits:**
    - Fully `constexpr` compatible.
    - Stack-allocated (fast).
    - Prevents accidental use for dynamic types (via constraints).

---

## 3. C++26 Reflection Strategies (`std::meta`)

### Fixed-Size Detection
Use recursive reflection to verify that all non-static data members are either fundamental types, enums, or other fixed-size structs.

### Padding & Layout
- **Memory-Map Style:** Uses `sizeof(T)` and `std::bit_cast`. Fast but includes padding bytes.
- **Packed Protocol Style:** Uses `template for` to iterate members and write them individually. More portable, removes padding, but requires manual size calculation.

---

## 4. Performance Summary

| Method | Allocation | Inlining | SIMD | Constexpr |
| :--- | :--- | :--- | :--- | :--- |
| **`pack_to` (Push)** | None | Yes | Likely | Yes |
| **`pack` (Array Return)** | Stack | Yes | Likely | Yes |
| **`pack` (Generator)** | Heap | No | No | No |

## 5. Next Steps for Proof-of-Concept
1. Implement `is_fixed_size<T>()` using `std::meta::nonstatic_data_members_of`.
2. Implement the core `pack_to` recursive loop.
3. Add the `requires`-constrained `std::array` return wrapper.


gemini --resume 3fb1f579-e465-48d3-be23-c93925e85081
