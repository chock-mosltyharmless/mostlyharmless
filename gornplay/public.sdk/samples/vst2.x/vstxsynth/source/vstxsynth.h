//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2006/11/13 09:08:27 $
//
// Category     : VST 2.x SDK Samples
// Filename     : vstxsynth.h
// Created by   : Steinberg Media Technologies
// Description  : Example VstXSynth
//
// A simple 2 oscillators test 'synth',
// Each oscillator has waveform, frequency, and volume
//
// *very* basic monophonic 'synth' example. you should not attempt to use this
// example 'algorithm' to start a serious virtual instrument; it is intended to demonstrate
// how VstEvents ('MIDI') are handled, but not how a virtual analog synth works.
// there are numerous much better examples on the web which show how to deal with
// bandlimited waveforms etc.
//
// © 2006, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#ifndef __vstxsynth__
#define __vstxsynth__

#include "public.sdk/source/vst2.x/audioeffectx.h"

// Use this define to save the music to file.
#define SAVE_MUSIC

// Size of a buffer with random numbers
#define RANDOM_BUFFER_SIZE 65536
#define DELAY_MULTIPLICATOR 128
#define MAX_DELAY_LENGTH (DELAY_MULTIPLICATOR * 130) // Some safety for miscalculation stuff...

// Number of additive overtones
#define NUM_OVERTONES 16
#define NUM_Stereo_VOICES 4

#ifndef PI
#define PI 3.1415926
#endif

//------------------------------------------------------------------------------------------
enum
{
	// Global
	kNumPrograms = 50,
	kNumOutputs = 2,

	kVolume = 0,
	kDuration,
	kAttack,
	kDecay,
	kSustain,
	kRelease,
	kQuakinessStart, // Volume of the overtones
	kQuakinessEnd,
	kSoundShapeStart, // Which random numbers to take for spectral shape
	kSoundShapeEnd,
	kModulationAmount, // Amount of Waveshape modulation
	kModulationSpeed, // Speed of the waveshape modulation
	kDetune, // Detuning of overtones
	kStereo, // Strength of Stereo (detune)
	kNoiseStart, // low-pass filter start frequency
	kNoiseEnd,
	kDelayFeed, // Resonance at the start
	kDelayLength,

	kNumParams
};

//------------------------------------------------------------------------------------------
// VstXSynthProgram
//------------------------------------------------------------------------------------------
class VstXSynthProgram
{
friend class VstXSynth;
public:
	VstXSynthProgram ();
	~VstXSynthProgram () {}

private:
	float fVolume;
	float fDuration;
	float fAttack;
	float fDecay;
	float fSustain;
	float fRelease;
	float fQuakinessStart;
	float fQuakinessEnd;
	int iSoundShapeStart;
	int iSoundShapeEnd;
	float fModulationAmount;
	float fModulationSpeed;
	float fDetune;
	float fStereo;
	float fNoiseStart;
	float fNoiseEnd;
	float fDelayFeed;
	int iDelayLength;
	char name[kVstMaxProgNameLen+1];
};

//------------------------------------------------------------------------------------------
// VstXSynth
//------------------------------------------------------------------------------------------
class VstXSynth : public AudioEffectX
{
public:
	VstXSynth (audioMasterCallback audioMaster);
	~VstXSynth ();

	virtual void processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames);
	virtual VstInt32 processEvents (VstEvents* events);

	virtual void setProgram (VstInt32 program);
	virtual void setProgramName (char* name);
	virtual void getProgramName (char* name);
	virtual bool getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text);

	virtual void setParameter (VstInt32 index, float value);
	virtual float getParameter (VstInt32 index);
	virtual void getParameterLabel (VstInt32 index, char* label);
	virtual void getParameterDisplay (VstInt32 index, char* text);
	virtual void getParameterName (VstInt32 index, char* text);
	
	virtual void setSampleRate (float sampleRate);
	virtual void setBlockSize (VstInt32 blockSize);
	
	virtual bool getOutputProperties (VstInt32 index, VstPinProperties* properties);
		
	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual VstInt32 getVendorVersion ();
	virtual VstInt32 canDo (char* text);

	virtual VstInt32 getNumMidiInputChannels ();
	virtual VstInt32 getNumMidiOutputChannels ();

	virtual VstInt32 getMidiProgramName (VstInt32 channel, MidiProgramName* midiProgramName);
	virtual VstInt32 getCurrentMidiProgram (VstInt32 channel, MidiProgramName* currentProgram);
	virtual VstInt32 getMidiProgramCategory (VstInt32 channel, MidiProgramCategory* category);
	virtual bool hasMidiProgramsChanged (VstInt32 channel);
	virtual bool getMidiKeyName (VstInt32 channel, MidiKeyName* keyName);

private:
	float moogFilter(float frequency, float resonance, float in);
	float b0, b1, b2, b3, b4; // moog filter parameters

	float fVolume; // Overall volume of the instrument exluding midi velocity
	float fDuration;
	float fAttack;
	float fDecay;
	float fSustain;
	float fRelease;
	float fQuakinessStart; // Loudness of the overtones
	float fQuakinessEnd;
	int iSoundShapeStart; // Which random numbers to take for overtones
	int iSoundShapeEnd;
	float fNoiseStart;
	float fNoiseEnd;
	float fModulationAmount;
	float fModulationSpeed;
	float fDetune;
	float fStereo;
	float fDelayFeed;
	int iDelayLength;
	int iADSR; // 0: Attack, 2: Decay, 3: Release
	float fADSRVal; // Volume from ADSR envelope
	float fPhase[NUM_OVERTONES][NUM_Stereo_VOICES]; // Phase of the instrument
	float fModulationPhase; // Phase of the modulation
	float fTimePoint; // Time point relative to fDuration;
	float fLastOutput[NUM_Stereo_VOICES]; // Last output value
	float fRemainDC[NUM_Stereo_VOICES]; // To avoid clicking, this value contains the last output before start/stop
	float fScaler; // 2pi / sampleRate
	int sampleID; // ID of the processed sample

	// Random number generator
	float frand(void);
	unsigned long seed;
	float randomBuffer[RANDOM_BUFFER_SIZE];
	float lowNoise[RANDOM_BUFFER_SIZE];
	// The same as random Buffer, but contains exp(4*(randomBuffer-1))
	float expRandomBuffer[RANDOM_BUFFER_SIZE];
	float reverbBuffer[MAX_DELAY_LENGTH][NUM_Stereo_VOICES];
	int reverbBufferLength[NUM_Stereo_VOICES]; // Actual length taken for pull-out

	// Stuff for delayed midi notes
	int midiDelaySamples;
	VstInt32 midiDelayNote;
	VstInt32 midiDelayVelocity;

	VstXSynthProgram* programs;
	VstInt32 channelPrograms[16];

	VstInt32 currentNote;
	VstInt32 currentVelocity; // That is the midi volume

	void initProcess ();
	void noteOn (VstInt32 note, VstInt32 velocity);
	void noteOff ();
	void fillProgram (VstInt32 channel, VstInt32 prg, MidiProgramName* mpn);

#ifdef SAVE_MUSIC
	static int firstNoteTime; // This one is static along buffers so that streams are synchronized
	int savedNoteID;
	int savedNoteTime[1000000];
	int savedNote[1000000];
	int savedVelocity[1000000];
	int savedInstrumentID;
	int savedInstrumentTime[1000000]; // Made relative to firstNoteTime
	int savedInstrumentParameter[kNumParams][1000000];
#endif
};

#endif
