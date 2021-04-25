
// https://github.com/RobTillaart/MCP_ADC
#include "MCP_ADC.h"
// https://github.com/max22-/ESP32-BLE-MIDI
#include <BLEMidi.h>

#define NUM_ADCS 3
#define ADC_CHANNELS 8
#define PADS 24
#define ANALOG_NOTE_ON_THRESHOLD 200
#define ANALOG_NOTE_OFF_THRESHOLD 100
#define NOTE_OFFSET 60

MCP3008 mcp1, mcp2, mcp3; 
MCP3008 *mcps[] = {&mcp1, &mcp2, &mcp3};

uint16_t readings[PADS];
uint8_t notes[PADS];

void setup()
{
  Serial.begin(115200);
  Serial.println(__FILE__);

  BLEMidiServer.begin("Twister");

  // Chip select pins are hardwired from ESP-32 to each MCP3008
  mcp1.begin(17);
  mcp2.begin(21);                
  mcp3.begin(22);

  for(int i = 0; i < NUM_ADCS; i++) {
    mcps[i]->setSPIspeed(4000000);
  }
}

void loop()
{
  // Don't bother doing anything if we aren't connected.
  if(!BLEMidiServer.isConnected()) {
    delay(250);
    return;
  }

  // read all 24 channels into the 'readings' array
  for(int i = 0; i < NUM_ADCS; i++) {
    MCP3008 *mcp = mcps[i];
    int offset = i * ADC_CHANNELS;
    for (int channel = 0; channel < ADC_CHANNELS; channel++){
      uint16_t reading = mcp->analogRead(channel);
      readings[channel + offset] = reading;
      delay(1);       
    }
  }

  // for all readings, send MIDI events for every transition from on -> off or off -> on
  for(int n = 0; n < PADS; n++) {
    uint16_t reading = readings[n];
    if (reading > ANALOG_NOTE_ON_THRESHOLD && notes[n] == 0) {
      uint8_t velocity = min(reading << 2, 127);
      BLEMidiServer.noteOn(0, n + NOTE_OFFSET, velocity);
      notes[n] = velocity;
    } else if (reading < ANALOG_NOTE_OFF_THRESHOLD && notes[n] > 0) {
      BLEMidiServer.noteOff(0, n + NOTE_OFFSET, 127);
      notes[n] = 0;
    }
  }
}
