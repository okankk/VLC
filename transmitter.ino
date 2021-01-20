/*
************************************************************************
* Visible Light Communication System
* -------------------
* Author: Okan Kaya, 
* Revision: 1.0 
* Tapir Lab.
* Copyright: Okan Kaya, Tapir Lab., Istanbul Commerce University
************************************************************************
*/

/*
 * VLC Transmitter
 * In this project, a Visible Light Communication (VLC) system is designed.
 * The purpose of this project is transmitting data via visible light
 * by using only Arduino UNOs. The second phase of the project is transmitting
 * 640x480 (or 320x240) video without using any other microcontroller/interface.
 * For this version, user should type 10 characters at once and then hit enter for transmission.
 * 
 * Hardware for the transmitter is given below;
 * Adafruit TTL Laser Diode is used to transmit symbols.
 * 
 * Hardware connection;
 * +5V ---> Red cable of the laser diode (Power supply for the laser)
 * Arduino D10 ---> Yellow cable of the laser diode (Laser is controlled via this pin) 
 * Arduino GND ---> Black cable of the laser diode 
 */

#include <QueueArray.h> // This library is included to manipulating queues (array version).

#define LASER_ON 1 //HIGH Symbol
#define LASER_OFF 0 //LOW Symbol

QueueArray <unsigned char> input_buffer;
QueueArray <unsigned char> symbol_buffer;

int laser_pin = 10; // The digital pin which is connected to the control pin of the laser.
unsigned char b = 0;
int tx_working = 0; // In order to preserve the ongoing transmission,
// it holds the state of the transmitter.
// The ongoing transmission should not be interrupted by user.

void setup()
{
  Serial.begin(115200);
  pinMode(laser_pin, OUTPUT);
  
  // TIMER 1 for interrupt frequency 1000 Hz:
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 64499;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
  TIMSK1 |= (1 << OCIE1A);
  sei();
}

// Bits are converted to symbols in here.
// Manchester coding is used and 1 is represented as HIGH-LOW and 0 as LOW-HIGH.
int convert_to_symbol(unsigned char input)
{
  for (int i = 7; i >= 0; i--)
  {
    if (input & (1 << i)) // If bit to transmit is 1;
    {
      symbol_buffer.push(LASER_ON);
      symbol_buffer.push(LASER_OFF);
    }
    else // If bit to transmit is 0;
    {
      symbol_buffer.push(LASER_OFF);
      symbol_buffer.push(LASER_ON);
    }
  }
  return 0;
}

int get_usr_msg() // User message is read here.
{
  if (Serial.available() > 0)
  {
    b = Serial.read();
    if (b != 10) // The enter character is ignored.
    {
      input_buffer.push(b); //User input is pushed for bit to symbol conversion.
    }
  }
}

void loop() 
{
  get_usr_msg();

  // If the transmitter is not busy and the user has entered 10 characters, transmission starts.
  // Before message signal, synchronization bits have been pushed to the symbol buffer.
  if (!tx_working && input_buffer.count() >= 10)
  {

    symbol_buffer.push(LASER_ON);
    symbol_buffer.push(LASER_ON);

    symbol_buffer.push(LASER_ON);
    symbol_buffer.push(LASER_ON);

    symbol_buffer.push(LASER_ON);
    symbol_buffer.push(LASER_ON);

    symbol_buffer.push(LASER_ON);
    symbol_buffer.push(LASER_ON);

    symbol_buffer.push(LASER_ON);
    symbol_buffer.push(LASER_ON);

    symbol_buffer.push(LASER_ON);
    symbol_buffer.push(LASER_ON);

    symbol_buffer.push(LASER_ON);
    symbol_buffer.push(LASER_ON);

    symbol_buffer.push(LASER_ON);
    symbol_buffer.push(LASER_ON);

    for (int i = 0; i < 10; i++)
    {
      convert_to_symbol(input_buffer.pop());
      // The message signal is pushed to input buffer for encoding.
    }
    tx_working = 1; // Transmitter is working.
  }
}

ISR(TIMER1_COMPA_vect)
{
  // If the transmitter is not busy and if there is a message signal;
  if (tx_working && !symbol_buffer.isEmpty())
  {
    unsigned char c = symbol_buffer.pop();

    // Symbols will be converted to the voltage.
    // Instead of Arduino's predefined functions, ports are controlled manually 
    // for faster manipulation.
    if (c == LASER_ON)
    {
      //digitalWrite(laser_pin, HIGH);
      PORTB |= 1 << PORTB2; // +5V applied to the laser.
    }
    else
    {
      //digitalWrite(laser_pin, LOW);
      PORTB &= ~(1 << PORTB2);
    }
  }
  else
  {
    //digitalWrite(laser_pin, LOW);
    PORTB &= ~(1 << PORTB2); // Close the last signal.
    tx_working = 0; // Transmission is completed, set the transmitter to the idle.
  }
}
