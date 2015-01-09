//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2006/11/13 09:08:27 $
//
// Category     : VST 2.x SDK Samples
// Filename     : vstxsynth.cpp
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

#include "vstxsynth.h"
#include "gmnames.h"

//-------------------------------------------------------------------------------------------------------
AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	return new VstXSynth (audioMaster);
}

//-----------------------------------------------------------------------------------------
// VstXSynthProgram
//-----------------------------------------------------------------------------------------
VstXSynthProgram::VstXSynthProgram ()
{
	fVolume    = .25f;
	fDuration = 0.2f;
	fAttack = 0.3f;
	fRelease = 0.2f;
	fQuakinessStart = .5f;
	fQuakinessEnd = 0.2f;
	iSoundShapeStart = 0;
	iSoundShapeEnd = 1;
	fModulationAmount = 0.1f;
	fModulationSpeed = 0.3f;
	fDetune = 0.0f;
	fStereo = 0.0f;
	fNoiseStart = 0.0f;
	fNoiseEnd = 0.0f;
	fDelayFeed = 0.0f;
	iDelayLength = 0;
	vst_strncpy (name, "Basic", kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
// VstXSynth
//-----------------------------------------------------------------------------------------
VstXSynth::VstXSynth (audioMasterCallback audioMaster)
: AudioEffectX (audioMaster, kNumPrograms, kNumParams)
{
	// initialize programs
	programs = new VstXSynthProgram[kNumPrograms];
	for (VstInt32 i = 0; i < 16; i++)
		channelPrograms[i] = i;

	if (programs)
		setProgram (0);
	
	if (audioMaster)
	{
		setNumInputs (0);				// no inputs
		setNumOutputs (kNumOutputs);	// 2 outputs, 1 for each oscillator
		canProcessReplacing ();
		isSynth ();
		setUniqueID ('nceG');			// <<<! *must* change this!!!!
	}

	initProcess ();
	suspend ();
}

//-----------------------------------------------------------------------------------------
VstXSynth::~VstXSynth ()
{
	if (programs)
		delete[] programs;
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setProgram (VstInt32 program)
{
	if (program < 0 || program >= kNumPrograms)
		return;
	
	VstXSynthProgram *ap = &programs[program];
	curProgram = program;
	
	fVolume    = ap->fVolume;
	fDuration = ap->fDuration;
	fAttack = ap->fAttack;
	fRelease = ap->fRelease;
	fQuakinessStart = ap->fQuakinessStart;
	fQuakinessEnd = ap->fQuakinessEnd;
	iSoundShapeStart = ap->iSoundShapeStart;
	iSoundShapeEnd = ap->iSoundShapeEnd;
	fModulationAmount = ap->fModulationAmount;
	fModulationSpeed = ap->fModulationSpeed;
	fDetune = ap->fDetune;
	fStereo = ap->fStereo;
	fNoiseStart = ap->fNoiseStart;
	fNoiseEnd = ap->fNoiseEnd;
	fDelayFeed = ap->fDelayFeed;
	iDelayLength = ap->iDelayLength;
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setProgramName (char* name)
{
	vst_strncpy (programs[curProgram].name, name, kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getProgramName (char* name)
{
	vst_strncpy (name, programs[curProgram].name, kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterLabel (VstInt32 index, char* label)
{
	switch (index)
	{
		case kVolume:
			vst_strncpy (label, "dB", kVstMaxParamStrLen);
			break;
		case kQuakinessStart:
		case kQuakinessEnd:
			vst_strncpy (label, "dB", kVstMaxParamStrLen);
			break;
		case kModulationAmount:
			vst_strncpy (label, "dB", kVstMaxParamStrLen);
			break;
		default: // No label
			label[0] = 0;
			break;
	}
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterDisplay (VstInt32 index, char* text)
{
	text[0] = 0;
	switch (index)
	{
		case kVolume:		dB2string (fVolume, text, kVstMaxParamStrLen);	break;
		case kDuration:		float2string (fDuration, text, kVstMaxParamStrLen); break;
		case kAttack:		float2string (fAttack, text, kVstMaxParamStrLen); break;
		case kRelease:		float2string (fRelease, text, kVstMaxParamStrLen); break;
		case kQuakinessStart: dB2string (fQuakinessStart * 2.0f, text, kVstMaxParamStrLen); break;
		case kQuakinessEnd: dB2string (fQuakinessEnd * 2.0f, text, kVstMaxParamStrLen); break;
		case kSoundShapeStart:   int2string (iSoundShapeStart, text, kVstMaxParamStrLen); break;
		case kSoundShapeEnd: int2string (iSoundShapeEnd, text, kVstMaxParamStrLen); break;
		case kModulationAmount: dB2string (fModulationAmount, text, kVstMaxParamStrLen); break;
		case kModulationSpeed: float2string (fModulationSpeed, text, kVstMaxParamStrLen); break;
		case kDetune: float2string(fDetune, text, kVstMaxParamStrLen); break;
		case kStereo: float2string(fStereo, text, kVstMaxParamStrLen); break;
		case kNoiseStart: float2string(fNoiseStart, text, kVstMaxParamStrLen); break;
		case kNoiseEnd: float2string(fNoiseEnd, text, kVstMaxParamStrLen); break;
		case kDelayFeed: float2string(fDelayFeed, text, kVstMaxParamStrLen); break;
		case kDelayLength: int2string(iDelayLength, text, kVstMaxParamStrLen); break;
	}
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterName (VstInt32 index, char* label)
{
	switch (index)
	{
		case kVolume: vst_strncpy (label, "Volume", kVstMaxParamStrLen);	break;
		case kDuration: vst_strncpy(label, "Duration", kVstMaxParamStrLen); break;
		case kAttack: vst_strncpy(label, "Attack", kVstMaxParamStrLen); break;
		case kRelease: vst_strncpy(label, "Release", kVstMaxParamStrLen); break;
		case kQuakinessStart: vst_strncpy (label, "QuakinessStart", kVstMaxParamStrLen);	break;
		case kQuakinessEnd: vst_strncpy(label, "QuakinessEnd", kVstMaxParamStrLen); break;
		case kSoundShapeStart: vst_strncpy (label, "SoundShapeStart", kVstMaxParamStrLen);	break;
		case kSoundShapeEnd: vst_strncpy(label, "SoundShapeEnd", kVstMaxParamStrLen); break;
		case kModulationAmount: vst_strncpy(label, "Mod. Amount", kVstMaxParamStrLen); break;
		case kModulationSpeed: vst_strncpy(label, "Mod. Speed", kVstMaxParamStrLen); break;
		case kDetune: vst_strncpy(label, "Detune", kVstMaxParamStrLen); break;
		case kStereo: vst_strncpy(label, "Stereo", kVstMaxParamStrLen); break;
		case kNoiseStart: vst_strncpy(label, "NoiseStart", kVstMaxParamStrLen); break;
		case kNoiseEnd: vst_strncpy(label, "NoiseEnd", kVstMaxParamStrLen); break;
		case kDelayFeed: vst_strncpy(label, "DelayFeed", kVstMaxParamStrLen); break;
		case kDelayLength: vst_strncpy(label, "DelayLength", kVstMaxParamStrLen); break;
	}
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setParameter (VstInt32 index, float value)
{
	VstXSynthProgram *ap = &programs[curProgram];
	switch (index)
	{
		case kVolume:		fVolume		= ap->fVolume		= value;	break;
		case kDuration: fDuration = ap->fDuration = value; break;
		case kAttack: fAttack = ap->fAttack = value; break;
		case kRelease: fRelease = ap->fRelease = value; break;
		case kQuakinessStart: fQuakinessStart = ap->fQuakinessStart = value;	break;
		case kQuakinessEnd: fQuakinessEnd = ap->fQuakinessEnd = value; break;
		case kSoundShapeStart: iSoundShapeStart = ap->iSoundShapeStart = (int)(value * 128.0f + 0.5f);	break;
		case kSoundShapeEnd: iSoundShapeEnd = ap->iSoundShapeEnd = (int)(value * 128.0f + 0.5f); break;
		case kModulationAmount: fModulationAmount = ap->fModulationAmount = value; break;
		case kModulationSpeed: fModulationSpeed = ap->fModulationSpeed = value; break;
		case kDetune: fDetune = ap->fDetune = value; break;
		case kStereo: fStereo = ap->fStereo = value; break;
		case kNoiseStart: fNoiseStart = ap->fNoiseStart = value; break;
		case kNoiseEnd: fNoiseEnd = ap->fNoiseEnd = value; break;
		case kDelayFeed: fDelayFeed = ap->fDelayFeed = value; break;
		case kDelayLength: iDelayLength = ap->iDelayLength = (int)(value * 128.0f + 0.5f); break;
	}
}

//-----------------------------------------------------------------------------------------
float VstXSynth::getParameter (VstInt32 index)
{
	float value = 0;
	switch (index)
	{
		case kVolume:		value = fVolume;	break;
		case kDuration: value = fDuration; break;
		case kAttack: value = fAttack; break;
		case kRelease: value = fRelease; break;
		case kQuakinessStart:	value = fQuakinessStart;	break;
		case kQuakinessEnd: value = fQuakinessEnd; break;
		case kSoundShapeStart:	value = (float)iSoundShapeStart / 128.0f; break;
		case kSoundShapeEnd: value = (float)iSoundShapeEnd / 128.0f; break;
		case kModulationAmount: value = (float)fModulationAmount; break;
		case kModulationSpeed: value = (float)fModulationSpeed; break;
		case kDetune: value = fDetune; break;
		case kStereo: value = fStereo; break;
		case kNoiseStart: value = fNoiseStart; break;
		case kNoiseEnd: value = fNoiseEnd; break;
		case kDelayFeed: value = fDelayFeed; break;
		case kDelayLength: value = (float)iDelayLength / 128.0f; break;
	}
	return value;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getOutputProperties (VstInt32 index, VstPinProperties* properties)
{
	if (index < kNumOutputs && index >= 0)
	{
		vst_strncpy (properties->label, "Vstx ", 63);
		char temp[11] = {0};
		int2string (index + 1, temp, 10);
		vst_strncat (properties->label, temp, 63);

		properties->flags = kVstPinIsActive;
		if (index < 2)
			properties->flags |= kVstPinIsStereo;	// make channel 1+2 stereo
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text)
{
	if (index < kNumPrograms)
	{
		vst_strncpy (text, programs[index].name, kVstMaxProgNameLen);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getEffectName (char* name)
{
	vst_strncpy (name, "nce_GornPlay", kVstMaxEffectNameLen);
	return true;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getVendorString (char* text)
{
	vst_strncpy (text, "Nuance", kVstMaxVendorStrLen);
	return true;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getProductString (char* text)
{
	vst_strncpy (text, "Gorn Play", kVstMaxProductStrLen);
	return true;
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::getVendorVersion ()
{ 
	return 1000; 
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::canDo (char* text)
{
	if (!strcmp (text, "receiveVstEvents"))
		return 1;
	if (!strcmp (text, "receiveVstMidiEvent"))
		return 1;
	if (!strcmp (text, "midiProgramNames"))
		return 1;
	return -1;	// explicitly can't do; 0 => don't know
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::getNumMidiInputChannels ()
{
	return 1; // we are monophonic
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::getNumMidiOutputChannels ()
{
	return 0; // no MIDI output back to Host app
}

// midi program names:
// as an example, GM names are used here. in fact, VstXSynth doesn't even support
// multi-timbral operation so it's really just for demonstration.
// a 'real' instrument would have a number of voices which use the
// programs[channelProgram[channel]] parameters when it receives
// a note on message.

//------------------------------------------------------------------------
VstInt32 VstXSynth::getMidiProgramName (VstInt32 channel, MidiProgramName* mpn)
{
	VstInt32 prg = mpn->thisProgramIndex;
	if (prg < 0 || prg >= 128)
		return 0;
	fillProgram (channel, prg, mpn);
	if (channel == 9)
		return 1;
	return 128L;
}

//------------------------------------------------------------------------
VstInt32 VstXSynth::getCurrentMidiProgram (VstInt32 channel, MidiProgramName* mpn)
{
	if (channel < 0 || channel >= 16 || !mpn)
		return -1;
	VstInt32 prg = channelPrograms[channel];
	mpn->thisProgramIndex = prg;
	fillProgram (channel, prg, mpn);
	return prg;
}

//------------------------------------------------------------------------
void VstXSynth::fillProgram (VstInt32 channel, VstInt32 prg, MidiProgramName* mpn)
{
	mpn->midiBankMsb =
	mpn->midiBankLsb = -1;
	mpn->reserved = 0;
	mpn->flags = 0;

	if (channel == 9)	// drums
	{
		vst_strncpy (mpn->name, "Standard", 63);
		mpn->midiProgram = 0;
		mpn->parentCategoryIndex = 0;
	}
	else
	{
		vst_strncpy (mpn->name, GmNames[prg], 63);
		mpn->midiProgram = (char)prg;
		mpn->parentCategoryIndex = -1;	// for now

		for (VstInt32 i = 0; i < kNumGmCategories; i++)
		{
			if (prg >= GmCategoriesFirstIndices[i] && prg < GmCategoriesFirstIndices[i + 1])
			{
				mpn->parentCategoryIndex = i;
				break;
			}
		}
	}
}

//------------------------------------------------------------------------
VstInt32 VstXSynth::getMidiProgramCategory (VstInt32 channel, MidiProgramCategory* cat)
{
	cat->parentCategoryIndex = -1;	// -1:no parent category
	cat->flags = 0;					// reserved, none defined yet, zero.
	VstInt32 category = cat->thisCategoryIndex;
	if (channel == 9)
	{
		vst_strncpy (cat->name, "Drums", 63);
		return 1;
	}
	if (category >= 0 && category < kNumGmCategories)
		vst_strncpy (cat->name, GmCategories[category], 63);
	else
		cat->name[0] = 0;
	return kNumGmCategories;
}

//------------------------------------------------------------------------
bool VstXSynth::hasMidiProgramsChanged (VstInt32 channel)
{
	return false;	// updateDisplay ()
}

//------------------------------------------------------------------------
bool VstXSynth::getMidiKeyName (VstInt32 channel, MidiKeyName* key)
								// struct will be filled with information for 'thisProgramIndex' and 'thisKeyNumber'
								// if keyName is "" the standard name of the key will be displayed.
								// if false is returned, no MidiKeyNames defined for 'thisProgramIndex'.
{
	// key->thisProgramIndex;		// >= 0. fill struct for this program index.
	// key->thisKeyNumber;			// 0 - 127. fill struct for this key number.
	key->keyName[0] = 0;
	key->reserved = 0;				// zero
	key->flags = 0;					// reserved, none defined yet, zero.
	return false;
}
