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
 * VLC Receiver
 * In this project, a Visible Light Communication (VLC) system is designed.
 * The purpose of this project is transmitting data via visible light
 * by using only Arduino UNOs. The second phase of the project is transmitting
 * 640x480 (or 320x240) video without using any other microcontroller/interface.
 * For this version, user should type 10 characters at once and then hit enter for transmission.
 * 
 * 
 * Hardware for the receiver is given below;
 * OSRAM SFH203 is choosen as photodiode for the receiver,
 * 10 kilo Ohm resistor.
 * 
 * Hardware connection;
 * +5V ---> Photodiode (+) ---> Photodiode ---> Photodiode (-) ---> Resistor ---> Arduino GND
 *                                                             |
 *                                                             |
 * Arduino D7 pin(voltage across the resistor is measured) <---|
 */

#include <QueueArray.h> // This library is included to manipulating queues (array version).

QueueArray <unsigned char> bit_buffer; //An array for bit buffer is created via the library.

int pd_pin = 7; // The digital pin which is connected to the output of the photodiode.
int val = 0; // The value read from the photodiode.
int counter = 0; 
int data_counter = 0;
int symbol_counter = 0;
unsigned char buffer = 0;
char symbol_to_bit_translation[64] = {'z', 'z', 'z', '0', 'z', '0', '0', '0', 'z', 'z', 'z', '0', 'z', '0', '0', '0', 'z', 'z', 'z', '0', 'z', '0', '0', '0', '1', '1', '1', 'o', '1', 'o', 'o', 'o', 'z', 'z', 'z', '0', 'z', '0', '0', '0', '1', '1', '1', 'o', '1', 'o', 'o', 'o', '1', '1', '1', 'o', '1', 'o', 'o', 'o', '1', '1', '1', 'o', '1', 'o', 'o', 'o'};
/* 
 *  This lookup table is created to map the sequence of symbols to each bit.
 *  Each bit is encoded with 2 symbols and the receiver
 *  is working 3 times faster than the transmitter.
 *  So, receiver samples 6 symbols for each bit.(2 x 3 = 6)
 *  Therefore, lookup table contains 64 combinations.(2^6 = 64)
*/
unsigned char data = 0;
 
int reception_started = 0; // It holds the state of the receiver.
int rx_counter = 0;

void setup() 
{
  Serial.begin(115200);
  pinMode(pd_pin, INPUT);
  //The digital pin to which the photodiode is connected is set as the input.

  // According to the Nyquist rate, the receiver should work at least 2 times faster than the transmitter.
  // TIMER 1 for interrupt frequency 3000 Hz:
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 21499;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
  TIMSK1 |= (1 << OCIE1A);
  sei();
}

void loop()
{
  if (!bit_buffer.isEmpty()) // If a bit is detected, first it will be counted for synchronization.
  {
    int x = bit_buffer.pop(); // Last object is popped from the queue.
    if (!reception_started) // If receiver is waiting for the preamble;
    {
      if (x == 'o') // Ones (1) are accumulated for the synchronization.
      {
        rx_counter++;
        if (rx_counter == 8) // Synchronization is done, now decoding can start.
        {
          reception_started = 1;
          rx_counter = 0;
          Serial.println("Protocol Started");
        }
      }
      else // Any other bit combinations ignored.
      {
        rx_counter = 0;
      }
    }
    else // Preamble is detected and the message signal can be decoded here.
    {
      if(x == '0') // If received bit is 0;
      {
        data = (data << 1);
        data_counter++;
      }
      
      else if(x == '1') // If received bit is 1;
      {
        data = (data << 1) | 1;
        data_counter++;
      }
      else // If received bit is anything other than 0 or 1, acknowledge and terminate.
      {
        reception_started = 0;
        data_counter = 0;
        Serial.println((char)x);
        Serial.println("Protocol Terminated");
      }
      if(data_counter >= 8) // ASCII Table is used and each char is represented with 1 byte.
      {
        Serial.println((char)data);
        data_counter = 0;
      }
    }
  }
}

ISR(TIMER1_COMPA_vect)
{
  val = digitalRead(pd_pin); // The value read from the photodiode.
  buffer = ((buffer << 1) | val)& 0xff;
  counter++;
  if (counter >= 6) // Every 6 symbols represent 1 bit.
  {
    unsigned char c = symbol_to_bit_translation[buffer];
    // The value read is selected from the table.
    
    if (!reception_started && c != 'o')
    {
        return;
    }
    bit_buffer.push(c); // Created bit pushed to the loop to process.
    buffer = 0; // Clear for the next iteration.
    counter = 0; // Clear for the next iteration.
  }
}
