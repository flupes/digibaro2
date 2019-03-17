/*
    Write indexes coded on 24 bits with 8 bit CRC, inject errors,
    and read them back.
*/

// We use a Zero compatible board
#define Serial SERIAL_PORT_USBVIRTUAL

// compile with:
// pio ci .\test\RecordData --board=zeroUSB --lib lib/SPIMemory --lib lib/FastCRC --project-option="targets=upload" --keep-build-dir

// monitor with:
// pio device monitor --port COM5 --baud 115200

#include <FastCRC.h>
#include <SPIMemory.h>

const uint32_t kInvalidInt24 = 0x00FFFFFE;

SPIFlash flash(4);

FastCRC8 CRC8;

class Int24Crc8
{
  public:
    static uint32_t Create(uint32_t data)
    {
        // zero MSB
        data &= 0x00FFFFFF;
        // compute crc
        uint8_t *addr = (uint8_t *)(&data);
        uint8_t crc_write = crc8.maxim(addr, 3);
        // assemble code word
        return data | ((uint32_t)crc_write << 24);
    }

    static bool Check(uint32_t code)
    {
        uint8_t *addr = (uint8_t *)(&code);
        uint8_t crc_check = crc8.maxim(addr, 3);
        if (crc_check != (code >> 24))
        {
            // Serial.print("CRC8 do not match for Int24 ");
            // Serial.print(code, DEC);
            // Serial.println(" !");
            return false;
        }
        return true;
    }

    static uint32_t Data(uint32_t code) { return code & 0x00FFFFFF; }

  private:
    static FastCRC8 crc8;
};

FastCRC8 Int24Crc8::crc8;

uint32_t ReadCheckInt24(uint32_t addr)
{
    uint32_t first = flash.readLong(addr, false);
    uint32_t second = flash.readLong(addr + 4, false);
    bool first_crc = Int24Crc8::Check(first);
    bool second_crc = Int24Crc8::Check(second);
    if (!first_crc && !second_crc)
    {
        Serial.println("Both indexes CRC are wrong (cannot thrust either value)!");
        return kInvalidInt24;
    }
    if (!first_crc)
    {
        Serial.println("CRC for first index is wrong!");
        return Int24Crc8::Data(second);
    }
    if (!second_crc)
    {
        Serial.println("CRC for second index is wrong!");
        return Int24Crc8::Data(first);
    }
    if (first != second)
    {
        Serial.println(
            "Double double fault (CRC are correct but number do not match)!");
        return kInvalidInt24;
    }
    return Int24Crc8::Data(first);
}

bool DoubleWriteInt24(uint32_t addr, uint32_t value)
{
    uint32_t code = Int24Crc8::Create(value);

    // This version of the code corrupt the data
    // before writing it for sectors 2, 3, 4 and 5!
    uint32_t code2 = code;
    if (addr == 2 * KB(4) || addr == 4 * KB(4))
    {
        code &= 0xFFFFFFF0;
        code |= 0xA;
    }
    if (flash.writeLong(addr, code, true))
    {
        if (addr == 3 * KB(4) || addr == 4 * KB(4))
        {
            code2 &= 0xFFFFFFF0;
            code2 |= 0xA;
        }
        if (addr == 5 * KB(4))
        {
            code2 = Int24Crc8::Create(0xABABAA);
        }
        if (flash.writeLong(addr + 4, code2, true))
        {
            return true;
        }
    }
    return false;
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    flash.begin();

    // Prepare the first sectors for the test
    Serial.print("Erasing first 32K.. .");
    uint32_t start = micros();
    flash.eraseBlock32K(0);
    uint32_t stop = micros();
    Serial.print((stop - start) / 1000);
    Serial.println("ms.");

    uint32_t index = 0xABABAB;
    Serial.print("Testing with index = ");
    Serial.println(index);
    Serial.println();
    Serial.print("Writing indexes... ");
    start = micros();
    for (int i = 1; i <= 5; i++)
    {
        DoubleWriteInt24(i * KB(4), index);
    }
    stop = micros();
    Serial.print((stop - start) / 1000);
    Serial.println("ms.");

    for (int i = 1; i <= 5; i++)
    {
        index = ReadCheckInt24(i * KB(4));
        Serial.print("Index on sector ");
        Serial.print(i);
        Serial.print(" = ");
        Serial.println(index);
    }
}

void loop() {}