#include "flash_config.h"

#include "flash_debug.h"

#include "RTCZero.h"
#include "SPIMemory.h"

FlashDebug::FlashDebug() : flash_(0), rtc_(0) {}

uint32_t FlashDebug::SetFlash(SPIFlash *flash) {
  if (!flash) return 0;
  flash_ = flash;
  if (NextAvailableDebugAddr()) {
    Message(INIT);
    return addr_;
  } else {
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < 50; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(300);
      digitalWrite(LED_BUILTIN, LOW);
      delay(200);
    }
    return 0;
  }
}

bool FlashDebug::SetRTC(RTCZero *rtc) {
  rtc_ = rtc;
  return rtc->isConfigured();
}

bool FlashDebug::Message(MessageType type, uint8_t value, int16_t extra) {
  if (!flash_) return false;
  
  uint32_t seconds = 0;
  if (rtc_) {
    seconds = rtc_->getEpoch();
  }
  bool ok = flash_->writeULong(addr_, seconds);
  addr_ += 4;
  ok &= flash_->writeByte(addr_++, type);
  ok &= flash_->writeByte(addr_++, value);
  ok &= flash_->writeShort(addr_, extra);
  addr_ += 2;
  return ok;
}

uint32_t FlashDebug::NextAvailableDebugAddr() {
  addr_ = kDebugSectorStart * KB(4);
  while (addr_ < 512 * KB(4)) {
    // read 4 bytes / advance 8 bytes at a time
    uint32_t word = flash_->readLong(addr_);
    if (word == 0xFFFFFFFF) return addr_;
    addr_ += 8;
  }
  // Memory full
  // Should handle better this scenario...
  return 0;
}
