# DSP_TeensyAudio_F32
Collected blocks for use with OpenAudio/Tympan libraries with floating point.

This is a collection of dsp blocks, i.e. Teensy Arduino objects to work with the Chip Audette OpenAudio_ArduinoLibrary or the Tympan_Library.  Both of these are 32-bit floating-point extensions to the PJRC Teensy Audio Library which exchanges data in 16-bit data blocks.  The target processor currently has been the PJRC Teensy 3.6 with testing to come soon for the Teensy 4.0.

My use of these blocks is for radio receivers and transmitter.  The goal is to be able to put together full-featured radios with DSP functions for low-frequency i-f and audio fnctions performed by existing libraries augmented by the blocks here.  Until a radio, or two, is functioning this is quite experimental.  I try to note the status of each block in the .h include file of the block. Every attempt will be made to not change existing member function definitions, but that may happen, when really needed.  New member functions may be added without concern of breaking exsisting code, s that will happen.
