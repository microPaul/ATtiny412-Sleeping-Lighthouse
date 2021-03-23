
/////////////////////////////////////////////
//
//  ATtiny412 Sleeping Lighthouse
//
//
//  Version: 210322-01
//
//  Author:  Paul Newland
//
//  Website: ad7i.net
//
//  Github:  microPaul
//
//  The purpose of this project is to simulate the light output of a
//  lighthouse (based loosely on the lighting pattern of the
//  Point No Point Lighthouse on Puget Sound) that produces a group
//  of three flashes followed by an off period of several seconds.
//  Basically, this program is an alternative blink sketch, except
//  that when the LED is off the MCU will be in the POWER DOWN sleep
//  state, drawing typically less than 1 microampere from the battery.
//  
//  This code was written for the Arduino IDE using Spence Konde's
//  megaTinyCore ( https://github.com/SpenceKonde/megaTinyCore ) and
//  pyupdi style programming with a USB-serial adapter and 4.7K resistor.
//
//
// Details:  Detailed information regarding thie program can be found in
//           in the readme file on GitHub
//
/////////////////////////////////////////////

#include <avr/sleep.h>

#define  F_CPU  4000000 // 4 MHz


volatile uint8_t rtcIntSemaphore;  // flag from RTS interrupt that may be used by polled function


///////////////////////////////////////////////////////////
// setup
//
// This is the "run it once at the start" code to setup the system.
//
void setup() {
  //Serial.swap(1); // need UART pins in swap position to leave DAC available

  initSerialGPIO(); // initialize serial and GPIO (serial is commented out)
  //Serial.println(); // print blank line
  //Serial.println("Sleeping Lighthouse signon msg"); 

  init32kOscRTCPIT(); // init the 32K internal Osc and RTC-PIT for interrupts
  initSleepMode();  // set up the sleep mode

  // now show some diagnostic data to confirm that the MCU system
  // speed is 4 MHz and not 20 MHz.  At 4 MHz the MCU should run
  // correctly from 1.8 VDC to 5.5 VDC.
  //
  //  delay(1000);
  //  flashByte(FUSE.OSCCFG);  // flash the value, should be 0x01 for 16 MHz internal oscillator
  //  delay(2000);
  //  flashByte(CLKCTRL.MCLKCTRLB); // flash the value, should be 0x03 for divide clock by 4
  //  delay(2000);
 
}


///////////////////////////////////////////////////////////
// loop
//
// This is the Super Loop.  In the Arduino world this is really a function (not a loop)
// that is called repeatedly by the Arduino executive.  As such, any local variables
// will be lost each time when the function concludes.  If you want to retain local
// variables then wrap the loop code in a while(1) statement.
//
void loop() {
  while(1) {
    // this loop blinks the LED three times then a 3 second gap
    // the MCU will be in POWER DOWN sleep mode when the LED is off
    ledOn(); // turn on LED
    delay(100);
    ledOff(); // turn off LED
    sleepNCycles(3); // cycles are about 500ms each
    
    ledOn(); // turn on LED
    delay(100);
    ledOff(); // turn off LED   
    sleepNCycles(3); // cycles are about 500ms each
    
    ledOn(); // turn on LED
    delay(100);
    ledOff(); // turn off LED
    sleepNCycles(8); // cycles are about 500ms each
  }
}


//////////////////////////////////////////////////////////////////
//  ISR(RTC_PIT_vect)
//
//  Interrupt Service Routine for the RTC PIT interrupt
//
ISR(RTC_PIT_vect) {
  RTC.PITINTFLAGS = RTC_PI_bm;  // clear the interrupt flag   
  rtcIntSemaphore = 1; // mark to polled function that interrupt has occurred

}


//////////////////////////////////////////////////////////////////
//  initSerialGPIO()
//
//  initialize Serial port and all needed GPIO
//
void initSerialGPIO(void) {
  //Serial.begin(9600, SERIAL_8N1); 
  PORTA.DIRSET = PIN1_bm; //  set port to output for LED
  PORTA.OUTCLR = PIN1_bm; // turn off LED
  PORTA.DIRSET = PIN2_bm; //  set port A2 to output for scope diagnostic
  PORTA.OUTCLR = PIN2_bm; // set A2 output low
}


//////////////////////////////////////////////////////////////////
//  init32kOscRTCPIT()
//
//  initialize the internal ultra low power 32 kHz osc and periodic Interrupt timer
//
//  these two peripherals are interconnected in that the internal 32 kHz
//  osc will not start until a peripheral (PIT in this case) calls for it,
//  and the PIT interrupt should not be enabled unit it is confirmed that
//  the 32 kHz osc is running and stable
//
//  Note that there is ERRATA on the RTC counter,  see:
//  https://ww1.microchip.com/downloads/en/DeviceDoc/ATtiny212-214-412-414-416-SilConErrataClarif-DS80000933A.pdf
//
//  That document currently states "Any write to the RTC.CTRLA register resets the 15-bit prescaler
//  resulting in a longer period on the current count or period".  So if you load the prescaler
//  value and then later enable the RTC (both on the same register) you get a very long
//  (max?) time period.  My solution to this problem is to always enable the RTC in the same
//  write that sets up the prescaler.  That seems to be an effective work-around. 
//
void init32kOscRTCPIT(void) {
  _PROTECTED_WRITE(CLKCTRL.OSC32KCTRLA, CLKCTRL_RUNSTDBY_bm);  // enable internal 32K osc   
  RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; //Select 1.024 kHz from 32KHz Low Power Oscillator (OSCULP32K) as clock source
  RTC.PITCTRLA = RTC_PERIOD_CYC512_gc | RTC_PITEN_bm; //  Enable RTC-PIT with divisor of 512 (~500 milliseconds)
  while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSC32KS_bm));  // wait until 32k osc clock has stabilized
  while (RTC.PITSTATUS > 0); // wait for RTC.PITCTRLA synchronization to be achieved
  RTC.PITINTCTRL = RTC_PI_bm;  // allow interrupts from the PIT device  
}

//////////////////////////////////////////////////////////////////
//  initSleepMode()
//
//  this doesn't invoke sleep, it just sets up the type of sleep mode
//  to enter when the MCU is put to sleep by calling "sleep_cpu()"
//
void initSleepMode(void) {
  //SLPCTRL.CTRLA = SLPCTRL_SMODE_IDLE_gc; // set sleep mode to "idle"
  //SLPCTRL.CTRLA = SLPCTRL_SMODE_STDBY_gc; // set sleep mode to "standby"  
  SLPCTRL.CTRLA = SLPCTRL_SMODE_PDOWN_gc; // set sleep mode to "power down"
  SLPCTRL.CTRLA |= SLPCTRL_SEN_bm;  // enable sleep mode
}


//////////////////////////////////////////////////////////////////
//  sleepNCycles(uint16_t val)
//
//  cause system to go to sleep for N cycles, where each cycle
//  is about 500mS.  Note that processor does wakeup every 500 ms
//  but goes back to sleep almost immediately if it has not yet
//  done the desired number of cycles.
//
//  At 4 MHz MCU speed, the awake duration of each iternation in the
//  for loop below is approximately 1.5 microseconds.
//
void sleepNCycles(uint8_t val) {
  // first cycle may not be full cycle
  disableAllPeripherals(); // all off
  for (uint8_t i = 0; i < val ; i++) {
    //diagnosticPinLow(); // set low to show sleeping
    sleep_cpu();  // put MCU to sleep
    //diagnosticPinHigh(); // set high to show awake
  }
  // now awake, sleep cycles complete, continue on
  initSerialGPIO(); // initialize serial and GPIO 
}


//////////////////////////////////////////////////////////////////
//  disableAllPeripherals()
//
//  To achieve minimum current consumption during sleep, it's best to
//  disable everything that might draw current during sleep.
//  This function disables everything except the RTC-PIT and 32K
//  internal low power oscillator.  This function does not disable
//  the counter used by the Arduino framework to implement millis(),
//  although the clock that drives that counter is suspended during sleep.
//
void disableAllPeripherals(void) {
  PORTA.DIRCLR = PIN0_bm; //  set port A0 to input
  PORTA.DIRCLR = PIN1_bm; //  set port A1 to input
  PORTA.DIRCLR = PIN2_bm; //  set port A2 to input
  PORTA.DIRCLR = PIN3_bm; //  set port A3 to input
  PORTA.DIRCLR = PIN6_bm; //  set port A4 to input
  PORTA.DIRCLR = PIN7_bm; //  set port A5 to input        
        
  PORTA.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc; // disable input buffers
  PORTA.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
  //USART0.CTRLB &= ~USART_RXEN_bm & ~USART_TXEN_bm; // disable USART


}

//////////////////////////////////////////////////////////////////
//  LED set/clear/toggle functions
//
void ledOn(void) {PORTA.OUTSET = PIN1_bm;} // turn on active high LED on PA1
void ledOff(void) {PORTA.OUTCLR = PIN1_bm;} // turn off active high LED on PA1
void ledToggle(void) {PORTA.OUTTGL = PIN1_bm;} // toggle active high LED on PA1


//////////////////////////////////////////////////////////////////
//  Diagnostic pin set/clear/toggle functions
//
void diagnosticPinHigh(void) {PORTA.OUTSET = PIN2_bm;} // port output high
void diagnosticPinLow(void) {PORTA.OUTCLR = PIN2_bm;} // port output low
void diagnosticPinToggle(void) {PORTA.OUTTGL = PIN2_bm;} // port output toggle


//////////////////////////////////////////////////////////////////
//  flashByte
//
//  This function provides a diagnosic report via a single LED.
//  An LED is flashed short when a bit is zero and flashed long when
//  a bit is one.  Eight bits are displayed sequentially starting with
//  LSB and ending with MSB.
//
void flashByte(uint8_t val) {
  uint8_t i;
  for (i = 0; i < 8 ; i++) {
    ledOn(); // turn on LED
    (val & 1) ? delay(700) : delay (100);  // long delay for 1, short if zero
    ledOff(); // turn off LED
    delay(1000);
    val >>= 1; // right shift val by one bit
  }
}

