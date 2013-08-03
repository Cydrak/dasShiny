
struct APU {
  void power();
  
  void stepMixer();
  
  void stepBuffer(unsigned no);
  void checkBufferEnable(unsigned no);
  
  void stepVoice(unsigned no);
  void stepPCM8(unsigned no);
  void stepPCM16(unsigned no);
  void stepADPCM4(unsigned no);
  void stepPulse(unsigned no);
  void stepNoise(unsigned no);
  
  void stopVoice(unsigned no);
  void fillBuffer(unsigned no);
  void checkLength(unsigned no);
  void checkEnable(unsigned no);
  
  
  uint32 regControl();
  uint32 regOutputBias();
  uint32 regChannelControl();
  uint32 regChannelDest(unsigned no);
  uint32 regVoiceControl(unsigned no);
  
  void regControl(uint32 data, uint32 mask);
  void regOutputBias(uint32 data, uint32 mask);
  void regChannelControl(uint32 data, uint32 mask);
  void regChannelDest(unsigned no, uint32 data, uint32 mask);
  void regChannelLength(unsigned no, uint32 data, uint32 mask);
  
  void regVoiceControl(unsigned no, uint32 data, uint32 mask);
  void regVoiceSource(unsigned no, uint32 data, uint32 mask);
  void regVoicePeriod(unsigned no, uint32 data, uint32 mask);
  void regVoiceLength(unsigned no, uint32 data, uint32 mask);
  
  
  struct Voice {
    uint1 enable, hold, playing;
    uint2 format;  enum { PCM8=0, PCM16=1, ADPCM4=2, PSG=3 };
    uint3 duty;
    uint2 limit;   enum { continuous=0, looped=1, once=2 };
    uint7 panning;
    
    // Volume is some kind of quasi-floating point format.
    // The effective value is base * 2^-(4 >> (3-exponent)).
    uint7 volumeBase;
    uint2 volumeExp;
    
    struct {
      uint32 source;
      uint16 counter;
      uint16 length1;
      uint32 length2;
    } init;
    
    struct {
      uint32 source;
      uint32 length;
      int16  sample;
      int16  state;
    } loop;
    
    uint32 source;
    uint32 length;
    uint16 counter;
    uint5  index;      // nibble index into buffer
    uint32 buffer[4];  // holds 8, 16, or 32 buffered samples
    int16  state;      // used for pulse, noise, and ADPCM
    
    int32  amplitude;
    int16  sample;
    
    Event  event;
  } voices[16];
  
  struct Channel {
    int64  sample;         // mixer output
    
    // Routing for audio buffers played on voice 1/3.
    uint1  toStream;       // mix with stream audio (voice 0/2)
    uint1  toMixer;        // mix with other voices (default)
    uint1  toLeftOutput;   // route to left audio output (default: use mixer)
    uint1  toRightOutput;  // route to right audio output
    
    // Enables buffering audio to RAM, for echo or filtering effects.
    uint1  toBuffer, buffering;
    uint1  format;         enum { PCM16=0, PCM8=1 };
    uint1  limit;          enum { looped=0, once=1 };
    uint1  source;         enum { mixer=0, stream=1 };
    
    struct {
      uint32 dest, length;
      uint16 counter;
    } init;
    
    uint32 dest, length;
    uint16 counter;
    uint5  index;      // nibble index into buffer
    uint32 buffer[4];  // holds 8 or 16 buffered samples
    
    Event  event;
  } channels[2];
  
  uint1  enable, powered;
  uint10 bias;
  uint7  volume;
  Event  mixEvent;
};

extern APU apu;
