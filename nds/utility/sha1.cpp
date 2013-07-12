
static inline uint32_t rol(uint32_t x, int n) {
  return n? x<<n | x>>32-n : x;
}

struct sha1ctx { uint32_t v[5]; };

template<int M> static inline void sha1round(uint32_t *t, uint32_t w) {
  w += t[4] + rol(t[0], 5);
  
  if(M == 0) w += t[1] & t[2] | ~t[1] & t[3];
  if(M == 1) w += t[1] ^ t[2] ^ t[3];
  if(M == 2) w += t[1] & t[2] | t[1] & t[3] | t[2] & t[3];
  
  t[4] = t[3];
  t[3] = t[2];
  t[2] = rol(t[1], 30);
  t[1] = t[0];
  t[0] = w;
}

string sha1(uint8_t *data, uint64_t size) {
  string result;
  sha1ctx hash  = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
  int64_t remain = size *= 8;
  size_t  chunks = (size+1 + 64 + 511) / 512;
  
  while(chunks--) {
    // Copy in up to 512 bits, terminating with 0x80.
    uint32_t w[80] = {0};
    for(int b = 0; b < 512; b += 8, remain -= 8)
      w[b/32] += remain > 0? *data++ << 24-b%32
               : remain < 0? 0 : 0x80 << 24-b%32;
    
    // If this is the last chunk, write length at the end.
    if(!chunks) w[14] = size>>32, w[15] = size;
    
    // Finally, expand the input before hashing.
    for(int n = 16; n < 80; n++)
      w[n] = rol(w[n-3] ^ w[n-8] ^ w[n-14] ^ w[n-16], 1);
    
    auto t = hash;
    for(int n =  0; n < 20; n++) sha1round<0>(t.v, w[n] + 0x5a827999u);
    for(int n = 20; n < 40; n++) sha1round<1>(t.v, w[n] + 0x6ed9eba1u);
    for(int n = 40; n < 60; n++) sha1round<2>(t.v, w[n] + 0x8f1bbcdcu);
    for(int n = 60; n < 80; n++) sha1round<1>(t.v, w[n] + 0xca62c1d6u);
    
    for(int n = 0; n < 5; n++)   hash.v[n] += t.v[n];
  }
  for(auto w : hash.v) result.append(hex<8>(w));
  return result;
}
