/* ReceiverPart2.ino    Bob Larkin   29 April 2020
 * This is a simple SP radio design.  It can receive 2 modes,
 * Single Sideband (SSB) and Narrow Band FM (NBFM).  SSB
 * breaks into Lower Sidband  (LSB) and Upper Sideband (USB).
 * It gets even better in that AM can be received on either
 * LSB or USB by tuning to the AM carrier frequency.
 * 
 * The signal path is switched between SSB and FM by a Chip
 * Audette AudioSwitch4_F32 switch block.  This keeps resources
 * from being used in blocks that are not needed.
 * 
 * We are restricted to receiving in the 8 to 22 kHz range,
 * so to use this on the air would require frequency conversion
 * hardware.
 *
 * Details on the blocks are part of the include .h files for the
 * blocks and in the INO files
 *   TestFM.ino
 *   ReceiverPart1.ino
 * 
 * Input and Output is via the Teensy Audio Adaptor that uses
 * a SGTL5000 CODEC,
 *
 * See the individual .h files for each block for more details.
 * 
 * Measured peak-to-peak levels, using AudioAnalyzePeak_F32,
 * all done at overload point for ADC:
 *   At ADC (i2sIn), max,  2.0
 *   At IQ Mixer out, max, 2.0
 *   At 90 deg Phase out,  2.0
 *   At Sum out            2.0
 *   At LPF FIR Out        2.0
 *   
 *   FM Det gives 0.50 out for about 5.6 kHz p-p deviation
 *   
 *   Processor load, measured: 16% for NBFM
 *                             30% for LSB or USB 29 tap LPF)
 */

#include "Audio.h"
#include <OpenAudio_ArduinoLibrary.h>

// *********  Mini Control Panel  *********
// Set mode and gain here and re-compile

// Here is the mode switch
#define LSB  1
#define USB  2
#define NBFM 3
uint16_t  mode = USB;   // <--Select mode

int gainControlDB = 0;  // Set SSB gain in dB. 0 dB is a gain of 1.0

// *****************************************
AudioInputI2S_F32           i2sIn;
AudioSwitch4_F32            switch1;    // Select SSB or FM
RadioIQMixer_F32            iqmixer1;
AudioFilter90Deg_F32        hilbert1;
AudioMixer4_F32             sum1;       // Summing node for the SSB receiver
AudioFilterFIR_F32          fir1;       // Low Pass Filter to frequency limit the SSB
RadioFMDetector_F32         fmdet1;     // NBFM from 10 to 20 kHz
AudioMixer4_F32             sum2;       // SSB and NBFM rejoin here
AudioOutputI2S_F32          i2sOut;
AudioAnalyzePeak_F32        peak1;
AudioControlSGTL5000        sgtl5000_1;

AudioConnection_F32     connect0(i2sIn,    0, switch1,  0);  // Analog to Digital
AudioConnection_F32     connect1(switch1,  0, iqmixer1, 0);  // SSB Input
AudioConnection_F32     connect2(switch1,  0, iqmixer1, 1);  // SSB Input
AudioConnection_F32     connect3(switch1,  1, fmdet1,   0);  // FM input
AudioConnection_F32     connect4(iqmixer1, 0, hilbert1, 0);  // Broadband 90 deg phase
AudioConnection_F32     connect5(iqmixer1, 1, hilbert1, 1);
AudioConnection_F32     connect6(hilbert1, 0, sum1,     0);   // Sideband select
AudioConnection_F32     connect7(hilbert1, 1, sum1,     1);
AudioConnection_F32     connect8(sum1,     0, fir1,     0);   // Limit audio BW
AudioConnection_F32     connect9(fir1,     0, sum2,     0);   // Output of SSB
AudioConnection_F32     connectA(fmdet1,   0, sum2,     1);   // Output of FM
AudioConnection_F32     connectC(sum2,     0, i2sOut,   0);   // Out to the CODEC left
AudioConnection_F32     connectD(sum2,     0, i2sOut,   1);   // and right
AudioConnection_F32     connectE(sum2,     0, peak1,   0);
// Filter for AudioFilter90Deg_F32 hilbert1 
#include "hilbert251A.h"

/* FIR filter designed with http://t-filter.appspot.com
 * fs = 44100 Hz, < 5kHz ripple 0.29 dB, >9 kHz, -62 dB, 29 taps
 */
float32_t fir_IQ29[29] = {
-0.000970689f, -0.004690292f, -0.008256345f, -0.007565650f,
 0.001524420f,  0.015435011f,  0.021920240f,  0.008211937f,
-0.024286413f, -0.052184700f, -0.040532507f,  0.031248107f,
 0.146902412f,  0.255179564f,  0.299445269f,  0.255179564f,
 0.146902412f,  0.031248107f, -0.040532507f, -0.052184700f,
-0.024286413f,  0.008211937f,  0.021920240f,  0.015435011f,
 0.001524420f, -0.007565650f, -0.008256345f, -0.004690292f,
-0.000970689f};

void setup(void) {
  float32_t vGain;
  AudioMemory(6);
  AudioMemory_F32(10);
  Serial.begin(300);  delay(1000);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();                     // ADC
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);

  // The switch is single pole 4 position, numbered (0, 3)  0=SSB, 1 = FM
  if(mode==LSB || mode==USB){ switch1.setChannel(0); Serial.println("SSB"); }
  else if(mode==NBFM)       { switch1.setChannel(1); Serial.println("NBFM"); }

  iqmixer1.frequency(15000.0);  // LO Frequency, typically 10 to 15 kHz

  hilbert1.begin(hilbert251A, 251);  // Set the Hilbert transform FIR filter

  sum1.gain(0, 1.0f);   // Leave set at 1.0
  if(mode==LSB)  sum1.gain(1, -1.0f);     // -1 for LSB out
  else if(mode==USB) sum1.gain(1, 1.0f);  // +1 for USB and for NBFM, we don't care

  // The FM detector has error checking during object construction
  // when Serial.print is not available.  See RadioFMDetector_F32.h:
  Serial.print("FM Initialization errors: ");
  Serial.println( fmdet1.returnInitializeFMError() );

  // See RadioFMDetector_F32.h for information on functions for modifying the
  // FM Detector.  Default values are used here, starting with a 15 kHz center frequency.
  
  fir1.begin(fir_IQ29, 29);           // 5 kHz bandwidth set for radio in SSB

  // The gainControlDB goes in 1 dB steps.  Convert here to a voltage ratio
  vGain = powf(10.0f, ((float32_t)gainControlDB)/20.0 );
  // And apply that ratio to the output summing block.  Gain here for SSB only
  sum2.gain(0, vGain);     Serial.print("vGain = "); Serial.println(vGain, 4);
  sum2.gain(1, 1.0f);       // FM gain

  // The following enable error checking inside of the blocks indicated.
  // Output goes to the Serial (USB) Monitor.  Use for debug.
  //hilbert1.showError(1);  // Should show input error when in FM
  //fmdet1.showError(1);    // Should show input error when in LSB or USB
}

void loop(void) {
// Here is where the adjustment of the volume control could go.
// And anything else that needs regular attention, other
// than the audio stream.

  if (peak1.available() ) {Serial.print("P-P ="); Serial.println(peak1.readPeakToPeak(), 6);}
  else Serial.println("Peak-Peak not available");
  Serial.print("CPU: Percent Usage, Max: ");
  Serial.print(AudioProcessorUsage());
  Serial.print(", ");
  Serial.print(AudioProcessorUsageMax());
  Serial.print("    ");
  Serial.print("Int16 Memory: ");
  Serial.print(AudioMemoryUsage());
  Serial.print(", ");
  Serial.print(AudioMemoryUsageMax());
  Serial.print("    ");
  Serial.print("Float Memory: ");
  Serial.print(AudioMemoryUsage_F32());
  Serial.print(", ");
  Serial.println(AudioMemoryUsageMax_F32());
  Serial.println();

  delay(1000);
} 
