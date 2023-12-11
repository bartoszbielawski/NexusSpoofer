#include <Arduino.h>
#include <vector>
#include <random>

struct NexusFrame
{
  uint32_t hi;
  uint32_t lo;
};


NexusFrame prepareNexusFrame(uint8_t id, bool battery, uint8_t channel, float temperature, uint8_t humidity)
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

  int16_t ft = round(temperature * 10.0f);  
  if (temperature < 0)
    ft = ~(-ft);

  l <<= 12;  //12 it temperature
  l |= ft & 0xFFF;    

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

struct NexusSensorId
{
  NexusSensorId(uint8_t id, uint8_t channel, 
    float t = 10.0f, float h = 50.0f): 
      id(id), channel(channel), temperature(t), humidity(h)
  {

  }

  uint8_t id;
  uint8_t channel;
  float temperature;
  float humidity;
};

static std::vector<NexusSensorId> spoofedSensors = {
  {0xA8, 0, 25.0f, 50.0f}, 
  {0x1B, 1, 10.0f, 30.0f},
  {0xBB, 2, -10.0f, 30.0f}, 
};

std::random_device rd{};
std::mt19937 gen{rd()};
std::normal_distribution<float> d(0, 0.4);

void loop() {
  for (auto& sensor: spoofedSensors)
  {    
    sensor.temperature += d(gen);
    sensor.humidity += d(gen);
  
    printf("%2X/%d: T: %.1f H: %.1f\n", sensor.id, sensor.channel, sensor.temperature, sensor.humidity);

    const auto nf = prepareNexusFrame(sensor.id, 1, sensor.channel, sensor.temperature, sensor.humidity);
    printNexusFrame(nf);
    playNexusFrame(nf, 5);  
  }
  
  delay(5000);
  digitalWrite(2, led);
  led = !led;
}
