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
	fADSRSpeed[0] = 0.05f;
	fADSRSpeed[1] = 0.05f;
	fADSRSpeed[2] = 0.05f;
	fVolume[0] = 0.0f;
	fVolume[1] = 0.5f;
	fVolume[2] = 0.3f;
	fVolume[3] = 0.0f;
	fQuak[0] = 0.1f;
	fQuak[1] = 0.3f;
	fQuak[2] = 0.1f;
	fQuak[3] = 0.0f;
	iShape[0] = 0;
	iShape[1] = 1;
	iShape[2] = 2;
	iShape[3] = 3;
	fDistort[0] = 0.6f;
	fDistort[1] = 0.8f;
	fDistort[2] = 0.2f;
	fDistort[3] = 0.0f;
	fNoise[0] = 0.0f;
	fNoise[1] = 0.0f;
	fNoise[2] = 0.0f;
	fNoise[3] = 0.0f;
	fDelayFeed = 0.0f;
	iDelayLength = 32;
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
	curProgram = &(programs[0]);
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
	
	//VstXSynthProgram *ap = &programs[program];
	curProgram = &(programs[program]);

#ifdef SAVE_MUSIC
	firstNoteTime = -1;
#endif
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setProgramName (char* name)
{
	vst_strncpy (curProgram->name, name, kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getProgramName (char* name)
{
	vst_strncpy (name, curProgram->name, kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterLabel (VstInt32 index, char* label)
{
	switch (index)
	{
		case kVolume1:
		case kVolume2:
		case kVolume3:
		case kVolume4:
		case kQuak1:
		case kQuak2:
		case kQuak3:
		case kQuak4:
			vst_strncpy (label, "dB", kVstMaxParamStrLen);
			break;
#if 0
		case kQuakinessStart:
		case kQuakinessEnd:
			vst_strncpy (label, "dB", kVstMaxParamStrLen);
			break;
		case kModulationAmount:
			vst_strncpy (label, "dB", kVstMaxParamStrLen);
			break;
#endif
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
		case kVolume1: dB2string (curProgram->fVolume[0], text, kVstMaxParamStrLen); break;
		case kVolume2: dB2string (curProgram->fVolume[1], text, kVstMaxParamStrLen); break;
		case kVolume3: dB2string (curProgram->fVolume[2], text, kVstMaxParamStrLen); break;
		case kVolume4: dB2string (curProgram->fVolume[3], text, kVstMaxParamStrLen); break;
		case kQuak1: dB2string (curProgram->fQuak[0], text, kVstMaxParamStrLen); break;
		case kQuak2: dB2string (curProgram->fQuak[1], text, kVstMaxParamStrLen); break;
		case kQuak3: dB2string (curProgram->fQuak[2], text, kVstMaxParamStrLen); break;
		case kQuak4: dB2string (curProgram->fQuak[3], text, kVstMaxParamStrLen); break;
		case kShape1: int2string (curProgram->iShape[0], text, kVstMaxParamStrLen); break;
		case kShape2: int2string (curProgram->iShape[1], text, kVstMaxParamStrLen); break;
		case kShape3: int2string (curProgram->iShape[2], text, kVstMaxParamStrLen); break;
		case kShape4: int2string (curProgram->iShape[3], text, kVstMaxParamStrLen); break;
		case kDistort1: dB2string (curProgram->fDistort[0], text, kVstMaxParamStrLen); break;
		case kDistort2: dB2string (curProgram->fDistort[1], text, kVstMaxParamStrLen); break;
		case kDistort3: dB2string (curProgram->fDistort[2], text, kVstMaxParamStrLen); break;
		case kDistort4: dB2string (curProgram->fDistort[3], text, kVstMaxParamStrLen); break;
		case kNoise1: dB2string (curProgram->fNoise[0], text, kVstMaxParamStrLen); break;
		case kNoise2: dB2string (curProgram->fNoise[1], text, kVstMaxParamStrLen); break;
		case kNoise3: dB2string (curProgram->fNoise[2], text, kVstMaxParamStrLen); break;
		case kNoise4: dB2string (curProgram->fNoise[3], text, kVstMaxParamStrLen); break;
		case kAttack: float2string(curProgram->fADSRSpeed[0], text, kVstMaxParamStrLen); break;
		case kDecay: float2string(curProgram->fADSRSpeed[1], text, kVstMaxParamStrLen); break;
		case kRelease: float2string(curProgram->fADSRSpeed[2], text, kVstMaxParamStrLen); break;
		case kDelayFeed: float2string(curProgram->fDelayFeed, text, kVstMaxParamStrLen); break;
		case kDelayLength: int2string(curProgram->iDelayLength, text, kVstMaxParamStrLen); break;
	}
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterName (VstInt32 index, char* label)
{
	switch (index)
	{
		case kAttack: vst_strncpy (label, "Attack", kVstMaxParamStrLen); break;
		case kDecay: vst_strncpy (label, "Decay", kVstMaxParamStrLen); break;
		case kRelease: vst_strncpy (label, "Release", kVstMaxParamStrLen); break;
		case kVolume1: vst_strncpy (label, "Volume1", kVstMaxParamStrLen);	break;
		case kVolume2: vst_strncpy (label, "Volume2", kVstMaxParamStrLen);	break;
		case kVolume3: vst_strncpy (label, "Volume3", kVstMaxParamStrLen);	break;
		case kVolume4: vst_strncpy (label, "Volume4", kVstMaxParamStrLen);	break;
		case kQuak1: vst_strncpy (label, "Quak1", kVstMaxParamStrLen); break;
		case kQuak2: vst_strncpy (label, "Quak2", kVstMaxParamStrLen); break;
		case kQuak3: vst_strncpy (label, "Quak3", kVstMaxParamStrLen); break;
		case kQuak4: vst_strncpy (label, "Quak4", kVstMaxParamStrLen); break;
		case kShape1: vst_strncpy (label, "Shape1", kVstMaxParamStrLen); break;
		case kShape2: vst_strncpy (label, "Shape2", kVstMaxParamStrLen); break;
		case kShape3: vst_strncpy (label, "Shape3", kVstMaxParamStrLen); break;
		case kShape4: vst_strncpy (label, "Shape4", kVstMaxParamStrLen); break;
		case kDistort1: vst_strncpy (label, "Distort1", kVstMaxParamStrLen); break;
		case kDistort2: vst_strncpy (label, "Distort2", kVstMaxParamStrLen); break;
		case kDistort3: vst_strncpy (label, "Distort3", kVstMaxParamStrLen); break;
		case kDistort4: vst_strncpy (label, "Distort4", kVstMaxParamStrLen); break;
		case kNoise1: vst_strncpy (label, "Noise1", kVstMaxParamStrLen); break;
		case kNoise2: vst_strncpy (label, "Noise2", kVstMaxParamStrLen); break;
		case kNoise3: vst_strncpy (label, "Noise3", kVstMaxParamStrLen); break;
		case kNoise4: vst_strncpy (label, "Noise4", kVstMaxParamStrLen); break;
		case kDelayFeed: vst_strncpy(label, "Delay feed", kVstMaxParamStrLen); break;
		case kDelayLength: vst_strncpy(label, "Delay length", kVstMaxParamStrLen); break;
	}
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setParameter (VstInt32 index, float value)
{
	//VstXSynthProgram *ap = &programs[curProgram];
	VstXSynthProgram *ap = curProgram;
	switch (index)
	{
		case kAttack: ap->fADSRSpeed[0] = value; break;
		case kDecay: ap->fADSRSpeed[1] = value; break;
		case kRelease: ap->fADSRSpeed[2] = value; break;
		case kVolume1: ap->fVolume[0] = value; break;
		case kVolume2: ap->fVolume[1] = value; break;
		case kVolume3: ap->fVolume[2] = value; break;
		case kVolume4: ap->fVolume[3] = value; break;
		case kQuak1: ap->fQuak[0] = value; break;
		case kQuak2: ap->fQuak[1] = value; break;
		case kQuak3: ap->fQuak[2] = value; break;
		case kQuak4: ap->fQuak[3] = value; break;
		case kShape1: ap->iShape[0] = (int)(value * 127.0f + 0.5f); break;
		case kShape2: ap->iShape[1] = (int)(value * 127.0f + 0.5f); break;
		case kShape3: ap->iShape[2] = (int)(value * 127.0f + 0.5f); break;
		case kShape4: ap->iShape[3] = (int)(value * 127.0f + 0.5f); break;
		case kDistort1: ap->fDistort[0] = value; break;
		case kDistort2: ap->fDistort[1] = value; break;
		case kDistort3: ap->fDistort[2] = value; break;
		case kDistort4: ap->fDistort[3] = value; break;
		case kNoise1: ap->fNoise[0] = value; break;
		case kNoise2: ap->fNoise[1] = value; break;
		case kNoise3: ap->fNoise[2] = value; break;
		case kNoise4: ap->fNoise[3] = value; break;
		case kDelayFeed: ap->fDelayFeed = value; break;
		case kDelayLength: ap->iDelayLength = (int)(value * 127.0f + 0.5f); break;
	}

#ifdef SAVE_MUSIC
	// Do I have to check for program?
	if (index >= 0 && index < kNumParams && curProgram == 0)
	{
		if (savedInstrumentID > 0 && sampleID - savedInstrumentTime[savedInstrumentID-1] < 4134)
		{
			// overwrite last one
			savedInstrumentID--;
		}
		else
		{
			savedInstrumentTime[savedInstrumentID] = sampleID;
			for (int i = 0; i < kNumParams; i++)
			{
				savedInstrumentParameter[i][savedInstrumentID] = -1;
			}
		}
		savedInstrumentParameter[index][savedInstrumentID] = (int)(value * 127.0f + 0.5f);
		savedInstrumentID++;
		savedInstrumentParameter[kNumParams][1000000];
	}
#endif
}

//-----------------------------------------------------------------------------------------
float VstXSynth::getParameter (VstInt32 index)
{
	float value = 0;
	switch (index)
	{
		case kAttack: value = curProgram->fADSRSpeed[0]; break;
		case kDecay: value = curProgram->fADSRSpeed[1]; break;
		case kRelease: value = curProgram->fADSRSpeed[2]; break;
		case kVolume1: value = curProgram->fVolume[0]; break;
		case kVolume2: value = curProgram->fVolume[1]; break;
		case kVolume3: value = curProgram->fVolume[2]; break;
		case kVolume4: value = curProgram->fVolume[3]; break;
		case kQuak1: value = curProgram->fQuak[0]; break;
		case kQuak2: value = curProgram->fQuak[1]; break;
		case kQuak3: value = curProgram->fQuak[2]; break;
		case kQuak4: value = curProgram->fQuak[3]; break;
		case kShape1: value = (float)(curProgram->iShape[0]) / 127.0f; break;
		case kShape2: value = (float)(curProgram->iShape[1]) / 127.0f; break;
		case kShape3: value = (float)(curProgram->iShape[2]) / 127.0f; break;
		case kShape4: value = (float)(curProgram->iShape[3]) / 127.0f; break;
		case kDistort1: value = curProgram->fDistort[0]; break;
		case kDistort2: value = curProgram->fDistort[1]; break;
		case kDistort3: value = curProgram->fDistort[2]; break;
		case kDistort4: value = curProgram->fDistort[3]; break;
		case kNoise1: value = curProgram->fNoise[0]; break;
		case kNoise2: value = curProgram->fNoise[1]; break;
		case kNoise3: value = curProgram->fNoise[2]; break;
		case kNoise4: value = curProgram->fNoise[3]; break;
		case kDelayFeed: value = curProgram->fDelayFeed; break;
		case kDelayLength: value = (float)(curProgram->iDelayLength) / 127.0f; break;
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
