# ATtiny412 Sleeping Lighthouse
A Demonstration of the POWER DOWN Sleep Mode of the ATtiny412

## Introduction

This is a small program to demonstrate the POWER DOWN sleep mode of the ATtiny412.  In this mode of operation the average current consumed by my test board in POWER DOWN sleep was approximately 0.6 microamperes when running from two AA cells.  When the MCU is active the processor speed is configured to 4 MHz, to allow the MCU to run over the full voltage range.

It should be a straightforward task to modify this code to run on the ATTiny3216 as well as the AVR128DBxx MCUs.

Microchip application note *AN2515 AVR® Low-Power Techniques* is a good reference.  It can be found at:  http://ww1.microchip.com/downloads/en/Appnotes/AN2515-AVR-Low-Power-Techniques-00002515C.pdf

The purpose of this project is to simulate the light output of a lighthouse (based loosely on the lighting pattern of the Point No Point Lighthouse on Puget Sound) that produces a group of three flashes followed by an off period of several seconds.  Basically, this program is an alterntaive blink sketch, except that when the LED is off the MCU will be in the POWER DOWN sleep state, drawing typically less than 1 microampere from the battery.

This code was written for the Arduino IDE using Spence Konde's megaTinyCore ( https://github.com/SpenceKonde/megaTinyCore ) and pyupdi style programming with a USB-serial adapter and 4.7K resistor.

## Method

The method of this example is that the internal 32.7 KHz oscillator* will operate continually, even in POWER DOWN sleep mode, and that the RTC-PIT is configured to cause an interrupt approximately every 500 ms.  That interrupt (or any interrupt) will wake the MCU to resume code execution.  

_[*The internal 32.7 kHz oscillator is optimized for low current operation at the expense of accuracy (see sections 35.4 and 39.6 in the data sheet for details). Although accuracy of the internal 32.7 kHz oscillator at 3V 25C is +/-3%, which is pretty good, the frequency climbs to about 34.5 kHz at 5V 25C. Over the full voltage and temperature range the accuracy of the internal 32.7 kHz oscillator is estimated to be no worse then +/- 30%, which is a pretty wide swing.  If better accuracy is needed one should use an external 32,768 Hz crystal connected to the oscillator at pins TOSC1/2, however the current consumed during sleep will be greater than that of the internal 32.7 kHz oscillator.  Note: quartz crystal accuracy is almost always better than +/-100 ppm right out of the box; it’s easy with care to get to +/-25 ppm; and +/-1 ppm will probably require TXCO efforts.]_

Making use of that sleep mechanism, the program flashes an LED (125 ms on, 1.5 seconds off) three times, and then the LED will remain off for an additional 2.5 seconds.  When the LED is on the MCU is in the active (run) state.  When the LED is off, the MCU is almost always in the POWER DOWN sleep state.

When it’s time for the MCU to sleep the function sleepNCycles(uint16_t val) is called.  This function will cause the MCU to enter the sleep state for “val” iterations, with each iteration lasting about 500 milliseconds.  A for-loop is used within the function and the MCU does wake up momentarily to execute each iteration of the for-loop.  The MCU active time within the for-loop lasts about 1.5 microseconds when the MCU clock is 4 MHz.

Looking at the sleepNCycles function in more detail, it (1) disables all peripherals and port-pins (so don’t count on outputs remaining in a low-Z state during sleep) to ensure low current draw in POWER DOWN sleep mode, then (2) causes the MCU to go to sleep, (3) when the PIT interrupt occurs the MCU will awaken, increment the iteration counter and if less than “val” will go to sleep again.  On the iteration where the cycle counter is not less than “val”, the MCU will remain awake and all needed peripherals and port pins are re-initialized (perhaps with outputs assuming a different value than what they had been when the MCU went to sleep) and the function will complete and return to the code that called it.  During each iteration of the for-loop, the MCU will be in the active mode (not sleeping) for approximately 1.5 microseconds every 500 ms.  Note that if the MCU draws 3 mA during active mode at 3 VDC (about 2x higher than what the data sheet indicates) that the **average** contribution of the MCU current consumption due to this 1.5 uS period of active operation every 500 ms is approximately 9nA ( (1.5/500,000) * 3mA).

Looking further at this average active current due to the for-loop MCU active time, if the sleep-cycle-time was reduced from 500 ms to 100 ms per iteration of the for-loop, to provide more granularity of sleep duration, the average current due to the active time within the for-loop increases from 9nA to 45nA, both numbers being very small with respect to typical internal battery leakage current, meaning there is very little battery life penalty by moving from 500 ms per iteration to 100 ms per iteration.

Below are the tasks the program does before entering the sleep state.  First, it enables the 32.7 kHz internal oscillator.  Note that enabling that oscillator does not cause it to run.  Two things need to be in place to cause that oscillator to run: (a) the oscillator must be enabled and (b) some peripheral must make use of that oscillator. Second, it sets up the RTC-PIT to use the divide-by-32 output of the 32.7 kHz oscillator (this will meet criterion (b) listed above).  Third, it sets the RTC-PIT prescaler for the desired divisor.  Fourth, it checks that both the 32.7 kHz oscillator is running and that the RTC-PIT internal synchronization has occurred before proceeding further.  Fourth, it selects the mode of sleep that is desired (idle, standby, power down).  Fifth, it enables sleep mode (which doesn’t cause sleep but does make it allowable).  Sixth, it disables all peripherals that were enabled since reset that are not needed during sleep, to ensure lowest possible current consumption in the sleep state.  Finally, to enter sleep mode, it calls sleep_cpu() to put the MCU to sleep.

Note that the only way to exit sleep mode and resume program execution is for either an interrupt or a reset to be detected by the MCU.  If interrupts were not configured before entering the sleep state the MCU will never enter sleep (except for a reset).

-------------

## Testing Current Consumption

Measurement of small currents (0.1 to 10 uA) usually requires a good quality digital multimeter.  Below I describe how to qualify and use a cheap digital voltmeter (like the $15 DT9205A or DT9208A DVMs on Ebay) to measure the sleep current.  The first step is to get a rough idea of the accuracy of the volt meter at low voltages (5mV).  Even cheap meters are usually pretty good in this range, but it’s always best to double-check.

Wire the circuit shown in Figure 1 shown below.

<p align="center">
  <img src="https://user-images.githubusercontent.com/73540066/111998413-5a802b80-8af2-11eb-9ffd-bbe304714b73.png" />
</p>




  
Resistors with 5% tolerance are fine.  Measure and record the voltage from T1 to ground; it should be 1.5 to 1.6 VDC (if it’s lower than 1.45 the AA cell should be replaced).  Next, measure and record the voltage from TP2 to ground. Calculate the difference between the two measurements.  They should differ by no more than 50 mV (15 mV is typical).  If the meter fails this test then the input impedance is too low, and you will need to try a different meter.  Now set your volt meter to lowest DC voltage scale available (probably 200mV full scale) but not less than 20 mV full scale.  Measure the voltage from TP3 to Ground.  It should be about 11 mV.  If it’s outside the range of 8 to 15 mV then go find another meter, the one under test is not acceptable.  Next, measure the voltage from TP 1 to ground; it should be about 5 mV.  If it’s somewhere between 3 and 7 mV you are OK.  Just know that what the meter shows is equivalent to 5mV.  If the measurement is outside that range, then try another meter.
At this point, if your meter has passed all these tests, then it may be used for the following procedure.


Refer to the test circuit of Figure 2 shown below.

<p align="center">
  <img src="https://user-images.githubusercontent.com/73540066/111998431-5d7b1c00-8af2-11eb-8681-149d80f9314a.png" />
</p>

Ensure that there is a shorting shunt across PH2 and that the programmer is disconnected from PH3, the UPDI connector.  You should observe that the LED is producing three flashes and then staying off for three seconds.  Now reconnect the programmer and in function main() comment out the last sleepNCycles(xxx) statement, and right below that add “sleepNCycles (16)”.  This will cause the LED off time after the three flashes to be at around 8 seconds.  Recompile and download the code to the MCU.  Disconnect the programmer and observe that the time between the three flashes of the LED is more than 6 seconds (counting aloud “one-alligator, two-alligator” accuracy is good enough).  

Measure the voltage of the two cells in series, just to confirm it’s about 3 VDC.  Next, configure the meter for the lowest DC voltage scale as tested above and connect it to the metering points across PH2 as shown in Figure 1.  Watch the LED and right after the three flashes have completed remove the shorting shunt from PH2, note the volt meter reading, and restore the shorting shunt to PH2 (if you don’t re-install the shorting shunt before the sleepNCycles() function concludes, the MCU will crash when it wakes up for good, due to low VDD caused by the large IR voltage drop through R3).  Whatever value you read on the meter in millivolts is equal to hundreds of nano-amps flowing into the test board.  When I perform this test on my test board I read 6 mV on the meter, which means that 600 nanoamps (0.6 uA) of current is flowing into my test board.  If you measure more than 4 microamperes (40 mV on your meter) you might try disconnecting one leg of the 10 uF cap (if it’s an electrolytic) and see if that reduces the current.  Electrolytic capacitors are notorious current leakers.

If your current consumption test above was successful then reconnect the programmer, remove the added “sleepNCycles (16)” line, uncomment the sleepNCycles(xxx) line that you commented-out earlier, recompile and download. Disconnect the programmer.  Observe that the LED off time between the group of three flashes is again about three seconds.

If you were not able to achieve a current of less than 4 uA during POWER DOWN sleep, then there is probably some peripheral on the MCU that remains active during sleep mode and is consuming current.  Go back and re-examine the code to see if something was turned on at one point that wasn’t turned off before sleep.  It’s very important to disable the input buffers of any input GPIO ports before entering sleep, unless you expect an interrupt on that port pin to wake the process from sleep.  These can be cumulative for current consumption, so the more port pins that are disabled during sleep, the better.  If there is a load on a pin port that remains in output mode during sleep it may also consume current.

<p align="center">
---ooOoo---
</p>





