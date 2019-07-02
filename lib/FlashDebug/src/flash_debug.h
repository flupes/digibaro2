#ifndef DIGIBARO_FLASH_DEBUG
#define DIGIBARO_FLASH_DEBUG

#include <stdint.h>

class SPIFlash;
class RTCZero;

class FlashDebug {
 public:
  enum MessageType : uint8_t {
    NONE = 0,
    INIT,
    BOOT,
    STANDBY,
    WAKEUP,
    ERROR,
    WARNING,
    STEP
  };

  FlashDebug();
  uint32_t SetFlash(SPIFlash *flash);
  bool SetRTC(RTCZero *rtc);

  bool Message(MessageType type, uint8_t value = 0, int16_t extra = 0);

 private:
  uint32_t NextAvailableDebugAddr();
  SPIFlash *flash_;
  RTCZero *rtc_;
  uint32_t addr_;
  char buffer[24];
};

#endif
