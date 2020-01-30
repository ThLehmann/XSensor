/* 
  Jeti Sensor EX Telemetry C++ Library
  
  JetiExSerial - EX serial output implementation for AtMega328
  --------------------------------------------------------------------
  
  Copyright (C) 2015 Bernd Wokoeck
  
  Version history:
  0.90   11/22/2015  created
  0.92   12/11/2015  baud rate for 8 MHz Pro mini (thanks to Wolfgang/wiff)
  0.94   12/22/2015  Teensy 3.x support on Serial2
  0.95   12/23/2015  Refactoring
  0.96   02/21/2016  comPort number as parameter for Teensy
  1.00   01/29/2017  Some refactoring:
                     - Optimized half duplex control for AVR CPUs in JetiExHardwareSerialInt class (for improved Jetibox key handling)
                     - Reduced size of serial transmit buffer (128-->64 words) 
                     - Changed bitrates for serial communication for AVR CPUs (9600-->9800 bps)
                     - JETI_DEBUG and BLOCKING_MODE removed (cleanup)
  1.0.1  02/15/2017  Support for ATMega32u4 CPU in Leonardo/Pro Micro
                     GetKey routine optimized 
  1.0.3  07/14/2017  Allow all jetibox key combinations (thanks to ThomasL)
                     Disable RX at startup to prevent reception of receiver identification

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

**************************************************************/

#include "JetiExSerial.h"

// Teensy
/////////
#ifdef CORE_TEENSY 

  JetiExSerial * JetiExSerial::CreatePort( int comPort )
  {
    return new JetiExTeensySerial( comPort );
  } 

  JetiExTeensySerial::JetiExTeensySerial( int comPort ) : m_bNextIsKey( false )
  {
    switch( comPort )
    {
    default:
    case 2: m_pSerial = &Serial2; break;
    case 1: m_pSerial = &Serial1; break;
    case 3: m_pSerial = &Serial3; break;
    }
  }

  void JetiExTeensySerial::Init()
  {
     m_pSerial->begin( 9600, SERIAL_9O1 );
  }
  void JetiExTeensySerial::Send( uint8_t data, boolean bit8 )
  {
    uint32_t w = data | ( bit8 ? 0x100 : 0x000 );
    m_pSerial->write9bit(w);
  }
  uint8_t JetiExTeensySerial::Getchar(void) // you must modify \Arduino\hardware\teensy\avr\cores\teensy3\serial2.c in order to receive jeti keys 
  {                                         // refer to TeensyReadme.txt
    while( m_pSerial->available() > 0 )
    {
		  int c = m_pSerial->read();
      // Serial.print( c ); 
      // if( c == 0x70 || c == 0xb0 || c == 0xd0 || c == 0xe0 ) // filter for jetibox keys: Left = 0x70, down = 0xb0, up= 0xd0, right = 0xe0
      if( c != 0xf0 && (c & 0x0f) == 0 )   // check upper nibble
        return c;
    }
    return 0;
  }

#else

// ATMega
/////////

#include <avr/io.h>
#include <avr/interrupt.h>


// HARDWARE SERIAL
//////////////////
void JetiExAtMegaSerial::Init() // pins are unsued for hardware version
{
#if HW_TERM == 1
return;
#endif

  // init UART-registers
  // 20.12.2017 ThL modified to 9700 baud, Jeti says 9600-9800 Baud (nobody knows why)

  UCSRA = 0x00;
  // 04.01.2019 ThL, prescaler von 16 auf 8, für besseres Teilerverhältnis
  UCSRA = _BV(U2X0); // doppelte Speed
  UCSRB = _BV(UCSZ2) /* | _BV(RXEN) */  | _BV(TXEN);        // 0x1C: 9 Bit, RX disable, Tx enable
  UCSRC = _BV(UCSZ0) | _BV(UCSZ1) | _BV(UPM0) | _BV(UPM1) ; // 0x36: 9-bit data, 2 stop bits, odd parity
//  UCSRC = _BV(UCSZ0) | _BV(UCSZ1) | _BV(UPM0) | _BV(UPM1) | _BV(USBS0);  // 0x36: 9-bit data, 2 stop bits, odd parity
  // 04.01.2019 ThL, USBS0 für 2 Stopp Bit
  UCSRC = _BV(UCSZ0) | _BV(UCSZ1) | _BV(UPM0) | _BV(UPM1) | _BV(USBS0);  // 0x36: 9-bit data, 2 stop bits, odd parity

  // wormfood.net/avrbaudcalc.php 
#if F_CPU == 16000000L  // for the 16 MHz clock on most Arduino boards
  UBRRH = 0x00;
  // 04.01.2019 ThL, auf 9700 eingemessen
  UBRRL = 205; // 9700 Bit/s 0.2% error, BIT Länge 103,09uS
//  UBRRL = 207; // 9600 Bit/s 0.2% error, BIT Länge 104,17uS
#elif F_CPU == 8000000L   // for the 8 MHz internal clock (Pro Mini 3.3 Volt) 
  UBRRH = 0x00;
  UBRRL = 101; // 9700 Bit/s 0.2% error, BIT Länge 103,09uS
//  UBRRL = 103; // 9600 Bit/s 0.2% error, BIT Länge 104,17uS
#else
  #error Unsupported clock speed
#endif  

  // TX and RX pins goes high, when disabled
  pinMode( 0, INPUT_PULLUP );
  pinMode( 1, INPUT_PULLUP );

  // debug
  // pinMode( 13, OUTPUT);
  // digitalWrite(13, LOW ); 
}


// Interrupt driven transmission
////////////////////////////////
JetiExHardwareSerialInt * _pInstance = 0;   // instance pointer to find the serial object from ISR

// static function for port object creation
JetiExSerial * JetiExSerial::CreatePort( int comPort )
{
  return new JetiExHardwareSerialInt();
} 

void JetiExHardwareSerialInt::Init()
{
  JetiExAtMegaSerial::Init();

  // init tx ring buffer 
  //memset( (void*)m_txBuf, 0, sizeof( m_txBuf ) );
  m_txHeadPtr = m_txBuf;
  m_txTailPtr = m_txBuf;
  m_txNumChar = 0;

  // init rx ring buffer 
  //memset( (void*)m_rxBuf, 0, sizeof( m_rxBuf ) );
//  m_rxHeadPtr = m_rxBuf;
//  m_rxTailPtr = m_rxBuf;
//  m_rxNumChar = 0;

  m_bSending  = false;
  _pInstance  = this; // there is a single instance only
}

// 12.11.2017 ThL modified receivebuffer to one byte only
// Read key from Jeti box
boolean JetiExHardwareSerialInt::Getchar( uint8_t *Ch )
{
  if( !RxGetChar )
    return( false );
  *Ch = RxChar;

  cli();
  RxChar = 0;
  RxGetChar = false;
  sei();
  return( true );
}


// Send one byte  
void JetiExHardwareSerialInt::Send( uint8_t data, boolean bit8 )
{
#if HW_TERM == 1
return;
#endif

  cli();
  if( m_txNumChar < TX_RINGBUF_SIZE )
  {
     *m_txHeadPtr = data | (bit8 ? 0x0100 : 0x0000);                       // write data to buffer
      m_txNumChar++;                                                       // increase number of characters in buffer
      m_txHeadPtr = IncBufPtr( m_txHeadPtr, m_txBuf, TX_RINGBUF_SIZE );    // increase ringbuf pointer
  }
  else
  {
    // todo handle buffer overflow
    // digitalWrite( 13, HIGH ); 
  }

  // enable transmitter
  if( !m_bSending )
  {
    m_bSending    = true;
    uint8_t ucsrb = UCSRB;
    ucsrb        &= ~( (1<<RXEN) | (1<<RXCIE) ); // disable receiver and receiver interrupt
    ucsrb        |=    (1<<TXEN) | (1<<UDRIE);   // enable transmitter and tx register empty interrupt 
    UCSRB         = ucsrb;

    // digitalWrite( 13, HIGH ); // show transmission
  }
  sei();
}

// increment buffer pointer (todo: use templates for 8 and 16 bit versions of pointers)
volatile uint16_t * JetiExHardwareSerialInt::IncBufPtr( volatile uint16_t * ptr, volatile uint16_t * pRingBuf, size_t bufSize )
{
  ptr++;
  if( ptr >= ( pRingBuf + bufSize ) )
    return pRingBuf; // wrap around
  else
    return ptr;
}

/*
volatile uint8_t * JetiExHardwareSerialInt::IncBufPtr8( volatile uint8_t * ptr, volatile uint8_t * pRingBuf, size_t bufSize )
{
  ptr++;
  if( ptr >= ( pRingBuf + bufSize ) )
    return pRingBuf; // wrap around
  else
    return ptr;
}
*/

#if HW_TERM == 0  
// ISR - transmission complete 
ISR( USART_TX_vect )
{
  // enable receiver
  uint8_t ucsrb = UCSRB; 
  ucsrb        &= ~( (1<<TXEN) | (1<<TXCIE) ); // disable transmitter and tx interrupt when there is nothing more to send
  ucsrb        |=    (1<<RXEN) | (1<<RXCIE);   // enable receiver with interrupt
  UCSRB        = ucsrb;

  // clear receiver buffer
//  _pInstance->RxChar = 0;
//  _pInstance->m_rxHeadPtr = _pInstance->m_rxBuf;
//  _pInstance->m_rxTailPtr = _pInstance->m_rxBuf;
//  _pInstance->m_rxNumChar = 0;

  // digitalWrite( 13, LOW ); 
}


// ISR - send buffer empty
ISR( USART_UDRE_vect )
{
  if( _pInstance->m_txNumChar != 0 )
  {
    uint16_t txChar = *(_pInstance->m_txTailPtr);

    // handle bit 8
    if( txChar & 0x0100 )
      UCSRB |= (1<<TXB8); 
    else
      UCSRB &= ~(1<<TXB8);

    UDR = (uint8_t)(txChar & 0xFF);

    _pInstance->m_txNumChar--; 
    _pInstance->m_txTailPtr = _pInstance->IncBufPtr( _pInstance->m_txTailPtr, _pInstance->m_txBuf, JetiExHardwareSerialInt::TX_RINGBUF_SIZE );
  }
  else
  {
    // enable TX complete interrupt to get a signal for end of transmission
    uint8_t ucsrb = UCSRB; 
    ucsrb        &= ~(1<<UDRIE); 
    ucsrb        |=  (1<<TXCIE);
    UCSRB        = ucsrb;
    _pInstance->m_bSending = false;
  }
}

// ISR - receiver buffer full
// 12.11.2017 ThL modified receive
//                - added error handling
//                - receive 9 BIT frames as described in datasheed
//                - receivebuffer one byte only
ISR( USART_RX_vect )
{
 uint8_t Stat = UCSRA;
 uint8_t Bit8 = UCSRB;      // read 9th bit first, recommended by datasheed
 uint8_t c = UDR;           // read data reg BIT 0-7 

  // error ? frame | overflow | parity
  // Tastencode wird mit Bit8 = 0 gesendet
  if( Stat & 0b00011100 || Bit8 & 0b00000010 )
    return;
  _pInstance->RxChar = c;
  _pInstance->RxGetChar = true;
/*
  // if( c == 0x70 || c == 0xb0 || c == 0xd0 || c == 0xe0 ) // Left = 0x70, down = 0xb0, up= 0xd0, right = 0xe0
  if( c != 0xf0 && (c & 0x0f) == 0 )   // check upper nibble
    _pInstance->RxChar = c;
*/
}

/* Original
// ISR - receiver buffer full
ISR( USART_RX_vect )
{
  // uint8_t status = UCSR0A;
  // uint8_t bit8 = UCSR0B; // unused
  uint8_t c = UDR;
  // if( c == 0x70 || c == 0xb0 || c == 0xd0 || c == 0xe0 ) // Left = 0x70, down = 0xb0, up= 0xd0, right = 0xe0
  if( c != 0xf0 && (c & 0x0f) == 0 )   // check upper nibble
  {
    *(_pInstance->m_rxHeadPtr) = c;  // write data to buffer
    if( _pInstance->m_rxNumChar < _pInstance->RX_RINGBUF_SIZE )
	    _pInstance->m_rxNumChar++;       // increase number of characters in buffer
    _pInstance->m_rxHeadPtr = _pInstance->IncBufPtr8( _pInstance->m_rxHeadPtr, _pInstance->m_rxBuf, _pInstance->RX_RINGBUF_SIZE );    // increase ringbuf pointer
  }

  // if( status & FE0 ) // debug
  //  digitalWrite( 13, HIGH ); 
}
*/

#endif

#endif // CORE_TEENSY 
