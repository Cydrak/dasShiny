
uint16_t crc16(uint8_t* data, unsigned size, uint16_t initial) {
  uint16_t table[] = { 0xa001,0xf001,0xd801,0xcc01,0xc601,0xc301,0xc181,0xc0c1 };
  uint32_t crc = initial;
  
  for(unsigned i = 0; i < size; i++) {
    crc ^= data[i];
    for(int j = 7; j >= 0; j--)
      crc = crc>>1 ^ (crc&1) * (table[j] << j);
  }
  return crc;
}
