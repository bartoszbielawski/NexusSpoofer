#include <Arduino.h>

struct NexusFrame
{
  uint32_t hi;
  uint32_t lo;
};


NexusFrame prepareNexusFrame(uint8_t id, bool battery, uint8_t channel, int16_t temperature, uint8_t humidity)
{
  NexusFrame nf;
  nf.hi = (id & 0xF0) >> 4;

  uint32_t l = id & 0xF;
  l <<= 1;
  l |= battery;

  l <<= 1;   //0 - after the battery there's a single bit that reads zero
  //or 0

  l <<= 2;   //2 bit channel
  l |= channel & 3;    

  l <<= 12;  //12 it temperature
  l |= temperature & 0x3FF;    

  l <<= 4;
  l |= 0xF;  // 0b1111 field

  l <<= 8;
  l |= humidity;  
  
  nf.lo = l;
  return nf;
}

const static int PREAMBLE_US = 4000;
const static int HIGH_US = 500;
const static int LOW_ONE_US = 2000;
const static int LOW_ZERO_US = 1000;

void playNexusFrame(const NexusFrame& nexusFrame, uint8_t pin)
{
  for (int f = 0; f < 5; f++)
  { 
    digitalWrite(pin, 1);
    digitalWrite(4, 1);   
    delayMicroseconds(HIGH_US);  
    digitalWrite(pin, 0);
    digitalWrite(4, 0);
    delayMicroseconds(PREAMBLE_US);  
    auto hi = nexusFrame.hi;
    for (int i = 0; i < 4; i++)
    {
      digitalWrite(pin, 1);
      digitalWrite(4, 1);
      delayMicroseconds(HIGH_US);
      digitalWrite(pin, 0);  
      digitalWrite(4, 0);
      int bitDelay = hi & 0x8 ? LOW_ONE_US: LOW_ZERO_US;
      delayMicroseconds(bitDelay);  
      hi <<= 1;
    }

    auto lo = nexusFrame.lo;
    for (int i = 0; i < 32; i++)
    {
      digitalWrite(pin, 1);
      digitalWrite(4, 1);
      delayMicroseconds(HIGH_US);
      digitalWrite(pin, 0);  
      digitalWrite(4, 0);
      int bitDelay = (lo & 0x80000000UL) ? LOW_ONE_US: LOW_ZERO_US;
      delayMicroseconds(bitDelay);  

      lo <<= 1;
    }
  }
   digitalWrite(pin, 0);
   digitalWrite(4, 0);
}

void printNexusFrame(const NexusFrame& nf)
{
  Serial.printf("%01X%08X\n", nf.hi, nf.lo);
}

void setup() 
{
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(2, OUTPUT);

  pinMode(15, OUTPUT);
  digitalWrite(15, 0);

  Serial.begin(9600);
  
}

bool led = false;


void loop() {
  const auto nf = prepareNexusFrame(75, 1, 1, ~100, 95);
  printNexusFrame(nf);
  playNexusFrame(nf, 5);  
  
  delay(1000);
  digitalWrite(2, led);
  led = !led;
}
