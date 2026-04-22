
// default logic (match to next checksum region)
struct Message {
  std::uint16_t magic;
  [[=leyline::checksum_region{}]] 
  Payload payload;
  [[=leyline::checksum_field_crc32{}]]
  std::uint32_t crc;
};

// struct tag region
struct payload_region_tag {};
struct Message {
  std::uint16_t magic;
  [[=leyline::checksum_region<payload_region_tag>{}]] 
  Payload payload;
  [[=leyline::checksum_field_crc32<payload_region_tag>{}]]
  std::uint32_t crc;
};

// integer regions example will match to a particuar region id
struct Message {
  std::uint16_t magic;
  [[=leyline::checksum_region{.region_id = 1}]] 
  Payload payload;
  [[=leyline::checksum_field_crc32{.region_id = 1}]]
  std::uint32_t crc;
};

// where leyline::checksum_field_crc32 is actually:
struct crc32_ieee_tag {};
template<typename T>
using checksum_field_crc32 = checksum_field<crc32_ieee_tag, T>;
// which permits user defined checkum implementations
