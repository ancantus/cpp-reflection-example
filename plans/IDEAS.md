# Idea Backlog

- `[[=wire::skip]]`: Do not serialize this field.
- `[[=wire::reserved_bytes{N}]]`: Reserve fixed bytes for forward compatibility.
- `[[=wire::reserved_must_be{V}]]`: Reserved field must decode to a specific value.
- `[[=wire::align{N}]]`: Force wire alignment boundary.
- `[[=wire::pad_value{B}]]`: Set deterministic padding byte value.
- `[[=wire::since{V}]]`: Field/type appears starting at protocol version `V`.
- `[[=wire::until{V}]]`: Field/type is valid through protocol version `V`.
- `[[=wire::validate_range{min,max}]]`: Range-check field during unpack.
- `[[=wire::bit_order::msb0]]` (or `lsb0`): Explicit bit numbering policy for bit-level packing.
- `[[=wire::rename{"name"}]]`: Override reflected field name for metadata/schema output.
