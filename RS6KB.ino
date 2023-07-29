// RS6KB
//
// Copyright (c) 2023 Eric Schlaepfer
// This work is licensed under the Creative Commons Attribution-ShareAlike 4.0
// International License. To view a copy of this license, visit
// http://creativecommons.org/licenses/by-sa/4.0/ or send a letter to Creative
// Commons, PO Box 1866, Mountain View, CA 94042, USA.

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>

// IO pin assignments
// PB0 = PS/2 data (keyboard side)
// PB1 = PS/2 clock (keyboard side)
// PC0 = PS/2 data (RS/6000 side)
// PC1 = PS/2 clock (RS/6000 side)
// PD2 = Tied to PC1, used as edge-triggered interrupt input
// PD6 = Analog switch control data line
// PD7 = Analog switch control clock line

#define TIMEOUT (200)

#define STATE_IDLE (0)
#define STATE_XFER_HOST (1)
#define STATE_XFER_KEYB (2)

#define BIT_COUNT_HOST (12)
#define BIT_COUNT_KEYB (10)

volatile uint8_t state = STATE_IDLE;
uint8_t dbcount;
uint16_t data_shift;
volatile uint16_t data_out;
volatile bool data_ready = false;
volatile bool host = false;

volatile int timeout = 0;

inline void SetIntFalling()
{
  EICRA = _BV(ISC01);
}

inline void SetIntRising()
{
  EICRA = _BV(ISC00) | _BV(ISC01); 
}

inline void KeyDisconnect()
{
  PORTD |= 0xC0;
}

inline void KeyConnect()
{
  PORTD &= ~0xC0;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  DDRB = 0x00; // All inputs
  DDRC = 0x00; // All inputs
  DDRD = 0xC0 | _BV(3); // Control are outputs. Normally open
  PORTD = 0;
  KeyConnect(); // Set the switches on.

  // Configure PD2 as an interrupt, falling edge.
  EICRA = _BV(ISC01); 
  EIMSK = _BV(INT0);
  state = STATE_IDLE;
  sei();
  Serial.println("Begin.");
}

// Computes the parity bit for the PS/2 protocol
int CalcParity(uint16_t by)
{
  uint16_t i, sum = 0;
  for (i = 0; i < 8; i++) {
    sum += ((by >> i) & 1);
  }
  return (~sum & 1);
}

// Transmits a byte to the RS/6K side. Generates clocks
void SendByte(uint16_t by)
{
  int i;
  by |= (CalcParity(by) << 8) | 0xE00;
  DDRC = 0x1; // Start bit
  for (i = 0; i < 10; i++) {
    delayMicroseconds(20);
    DDRC |= 0x2; // Clock goes low
    delayMicroseconds(40);
    DDRC &= 0x1; // Clock goes high
    delayMicroseconds(20);
    DDRC = (~by >> i) & 1; // Set the new bit
  }
  delayMicroseconds(20);
  DDRC = 0x2; // Clock low, data high
  delayMicroseconds(75); // Wait for ack?
  DDRC = 0x0; // Clock high
  // Done
}

// Main state machine
SIGNAL(INT0_vect)
{
  uint8_t d = PINC & 0x1;

  #ifdef DEBUGGING
  PORTD |= _BV(3);
  PORTD &= ~_BV(3);
  #endif

  timeout = 0;
  switch(state) {
    case STATE_IDLE:
      if (d == 1) {
        state = STATE_XFER_HOST; // Host mode transfer
        SetIntRising(); // On a host mode transfer we look at the rising edges
        dbcount = 0;
        data_shift = 0;
      } else {
        state = STATE_XFER_KEYB; // Keyboard mode transfer
        dbcount = 0;
        data_shift = 0;
      }
      break;
    case STATE_XFER_HOST: // Host mode data bit transfer
      data_shift |= d << dbcount++; // Clock it in
      if (dbcount == BIT_COUNT_HOST) {
        // Shift it to drop start bit
        data_out = data_shift >> 1;
        host = true;
        data_ready = true;
        SetIntFalling();
        state = STATE_IDLE;
      }
      break;
    case STATE_XFER_KEYB: // Keyboard mode data bit transfer
      data_shift |= d << dbcount++; // Clock it in
      if (dbcount == BIT_COUNT_KEYB) {
        data_out = data_shift; // No start bit, that's the first edge that we already caught.
        host = false;
        data_ready = true;
        state = STATE_IDLE;
      }
      break;
    default:
      SetIntFalling();
      state = STATE_IDLE;
  }
}

// Check for the host to send the EF command
// then generate the magic response code
void ListenByte()
{
  if (data_ready) { // Wait for a data byte
    data_ready = false;
    if (host && ((data_out & 0xFF) == 0xEF)) { // The magical mystery command: 0xEF is Layout ID
      cli();
      KeyDisconnect();
      delayMicroseconds(600);
      SendByte(0xFA); // Acknowledge the command
      delayMicroseconds(400);
      SendByte(0xB0); // B0 = United States 101 key layout. (B1 is the World Trade 102 key layout)
      delayMicroseconds(400);
      SendByte(0xBF); // BF is a byte common to both layouts.
      KeyConnect(); // And we are done. Whew!
      state = STATE_IDLE;
      SetIntFalling();
      sei();
    }

  }
}

// Reset state machine if the clock stays low for too low
void DoClockLowTimeout()
{
  if (!(PINC & 0x2)) {
    timeout = 0;
  } else {
    if (timeout++ > TIMEOUT) {
      cli();
      PORTD |= _BV(3);
      delayMicroseconds(10);
      PORTD &= ~_BV(3);
      timeout = 0;
      SetIntFalling();
      state = STATE_IDLE;
      sei();
    }
  }
}

void loop() {
  ListenByte();
  DoClockLowTimeout();
}
