//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#include <math.h>
#include "mzk.h"
#include "music.h"

// Synth variables:
#define NUM_FILTERS 16
#define NUM_RUNS 3
#define FILTER_DEGREE 3
//const float FP_MASTER_LOUDNESS = 2 * 128.0f;
const float FP_MASTER_LOUDNESS = 1 * 128.0f;
//const float FP_MASTER_LOUDNESS = 1.0f;
float fPhase = 0.0f;
float baseTime = 1200.0f; // This will be derived from the current frequency...
float barkS1[2][NUM_FILTERS][NUM_RUNS][FILTER_DEGREE];
float barkS2[2][NUM_FILTERS][NUM_RUNS][FILTER_DEGREE];

#define logStepSize 9
//#define stepSize (1<<(logStepSize-1))
#define stepSize 512
const static float invMarkerIntFactor = /*0.69315f*/ 1.0f / 24.0f / stepSize;
const static float invIntFactor = /*0.69315f*/ 1.0f / 2.0f / stepSize;
static int intEnergies[2][NUM_FILTERS];
static float energies[2][NUM_FILTERS];

// The sample buffer:
#define MAX_SAMPLE_LENGTH (stepSize*256)
static float sample[numInstruments][MAX_SAMPLE_LENGTH];

static unsigned long seed = 0;

// create random value between -65534 and 65534?
int rand()
{
	unsigned long a = 214013;
	unsigned long c = 2531011;
	unsigned long m = 4294967296-1;
	seed = (seed * a + c) % m;
	return (seed >> 8) % 65535;
}

// TODO: Check implementation from somewhere else. Esp. %65535? Numeric recipies.
float frand()
{
	return (float)(rand()) * (1.0f/65536.0f);
}

float exp2jo(float f)
{
	__asm fld f;
	__asm fld st;
	__asm frndint;
	__asm fsub st(1), st;
	__asm fxch;
	__asm f2xm1;
	__asm fld1;
	__asm fadd;
	__asm fscale;
	__asm fstp st(1);
	__asm fstp f;

	return f;
}

// filter coefficients
const float barkB[16] = {
  {0.00701958985393152850f}, 
  {0.00850017798023808450f}, 
  {0.01018870261846352300f}, 
  {0.01187342833008687500f}, 
  {0.01439342923812600100f}, 
  {0.01857466397411173700f}, 
  {0.01940811383234793800f}, 
  {0.02521639614210385200f}, 
  {0.02851526275542052800f}, 
  {0.03669925540645341300f}, 
  {0.04479451647513724800f}, 
  {0.05598234607010965000f}, 
  {0.08247408321970860200f}, 
  {0.10731786843158948000f}, 
  {0.21095185908890340000f}, 
  {0.34753734096841910000f}, 
};
const float barkA2[16][3] = {
  {-1.98701703318709360000f, -1.99862967736478050000f, -1.98607723569226070000f}, 
  {-1.98762990846400460000f, -1.99436408766326400000f, -1.98266780827771830000f}, 
  {-1.98524394523290160000f, -1.99151192784697170000f, -1.97843704323149370000f}, 
  {-1.98159537441185570000f, -1.98833604406138930000f, -1.97343713467062850000f}, 
  {-1.97521949902425020000f, -1.98375721125680960000f, -1.96561677164027150000f}, 
  {-1.96395557804575870000f, -1.97644338118495070000f, -1.95252111160244610000f}, 
  {-1.95467428449939520000f, -1.96864347874908610000f, -1.94329610550085820000f}, 
  {-1.93338206855382030000f, -1.95488470525713480000f, -1.92074581781958600000f}, 
  {-1.90912364473397610000f, -1.93706874842440180000f, -1.89709835990539630000f}, 
  {-1.86648268966779310000f, -1.90943127290553720000f, -1.85561884995514650000f}, 
  {-1.80686260539231780000f, -1.86914637511910550000f, -1.80017404635342660000f}, 
  {-1.71622008134699100000f, -1.80926849958848110000f, -1.71838461953240510000f}, 
  {-1.54247526040443650000f, -1.71161666195082400000f, -1.56986357210400840000f}, 
  {-1.28014300580439920000f, -1.54819120190906780000f, -1.35278810949188520000f}, 
  {-0.58550687095846410000f, -1.25851268434113810000f, -0.86066827839486404000f}, 
  {-0.60787538906045724000f, 0.67683264844056845000f, 0.03211538242054011400f}, 
};
const float barkA3[16][3] = {
  {0.98724038823275606000f, 0.99863184661189974000f, 0.98609789069205411000f}, 
  {0.98844068341648528000f, 0.99454407233784181000f, 0.98304821438779366000f}, 
  {0.98731706318300783000f, 0.99227821937263816000f, 0.97969109434295898000f}, 
  {0.98587728383902706000f, 0.99033556832426561000f, 0.97634604126643965000f}, 
  {0.98323911583853330000f, 0.98791354796118214000f, 0.97134927206051858000f}, 
  {0.97851186093915721000f, 0.98423871161141430000f, 0.96307643371830365000f}, 
  {0.97815083550595927000f, 0.98292065893542191000f, 0.96143004296454859000f}, 
  {0.97153824606126182000f, 0.97784372376213025000f, 0.94998050735981732000f}, 
  {0.96818655727474678000f, 0.97454626926688437000f, 0.94349622477338657000f}, 
  {0.95893047518313734000f, 0.96729184765208154000f, 0.92746677543586153000f}, 
  {0.95002173965935566000f, 0.95984043143561659000f, 0.91168951168857271000f}, 
  {0.93760470413636310000f, 0.94961175011762933000f, 0.89000954409276378000f}, 
  {0.90692399972541815000f, 0.92658591856367511000f, 0.83922103414772753000f}, 
  {0.87978052953127717000f, 0.90330972740713933000f, 0.79224232372269443000f}, 
  {0.76274440881678207000f, 0.81364819527829957000f, 0.60183060644761543000f}, 
  {0.66342142118105607000f, 0.66849220688560385000f, 0.35953939835128129000f}, 
};

// put here your synth
void mzk_init( short *buffer )
{		
	// Generate samples:
	int firstFrame = 0;

	// remove the first diff (along frequencies) from voicIntEnergies
	for (int i = 0; i < sumSampleFrames; i++)
	{
		for (int j = 1; j < 2 * NUM_FILTERS; j++)
		{
			diffEnergies[i][j] += diffEnergies[i][j-1];			
		}
	}

	for (int instrument = 0; instrument < numInstruments; instrument++)
	{		
		int numFrames = sampleFrames[instrument];

		// The length between two markers (log domain)
		int markerLength = ((int)diffMarkers[firstFrame] + 128) << logStepSize;
		// Init the voicIntEnergies:
		for (int i = 0; i < 2 * NUM_FILTERS; i++)
		{
			intEnergies[0][i] = diffEnergies[firstFrame][i] << logStepSize;
			//intEnergies[1][i] = diffUnvoicEnergies[0][i] << logStepSize;
		}

		for(int i = 0; i < (numFrames-1) * stepSize; i++)
		{
			baseTime = exp2jo(markerLength * invMarkerIntFactor);

			// calculate the current index into the sample frame data
			int frameIndex = firstFrame + (i>>logStepSize) + 1;

			// update the markerLength:
			markerLength += diffMarkers[frameIndex];
			// update the voicIntEnergies and calculate voicEnergies:
			for (int j = 0; j < 2 * NUM_FILTERS; j++)
			{
				energies[0][j] = exp2jo(intEnergies[0][j] * invIntFactor);
				//energies[1][j] = exp2jo(intEnergies[1][j] * invIntFactor);			
				intEnergies[0][j] += diffEnergies[frameIndex][j];
				//intEnergies[1][j] += diffUnvoicEnergies[frameIndex][j];
			}

			// this is all very raw, there is no means of interpolation,
			// and we will certainly get aliasing due to non-bandlimited
			// waveforms. don't use this for serious projects...
			float out[2];
			if (fabsf(fPhase) < (1.0f/1024.0f))
			{
				out[1] = 1.0f;
			}
			else
			{
				out[1] = sin(fPhase) / fPhase;
			}	
			// noise:
			out[0] = 0.25f * (frand() - 0.5f);

			// Filter
			float input, result;
			float intermediate;
			int filter = 3;
			intermediate = 0.0f;

			for (int inputType = 0; inputType < 2; inputType++)
			{
				for (filter = 0; filter < NUM_FILTERS; filter++)
				{	
					input = out[inputType];
					for (int l = 0; l < NUM_RUNS; l++)
					{
						for (int k = 0; k < FILTER_DEGREE; k++)
						{							
							result = barkB[filter] * input + barkS1[inputType][filter][l][k];
							barkS1[inputType][filter][l][k] = barkS2[inputType][filter][l][k] - barkA2[filter][k] * result;
							barkS2[inputType][filter][l][k] = -barkB[filter] * input - barkA3[filter][k] * result;
								
							input = result;
						}		
					}

					intermediate += result * energies[inputType][15-filter]; // I store the stuff upside down (DUH!) << TODO!
				}			
			}					

			fPhase += 3.0f;
			if (fPhase > baseTime * 3.0f / 2.0f)
			{
				fPhase -= baseTime * 3.0f;
			}
	
			//buffer[2*i+0] = (int)(intermediate * FP_MASTER_LOUDNESS);
			//buffer[2*i+1] = (int)(intermediate * FP_MASTER_LOUDNESS);
			sample[instrument][i] = intermediate * FP_MASTER_LOUDNESS;

	        //const float fl = sinf( 6.2831f*440.0f * (float)i/(float)MZK_RATE );
		    //const float fr = sinf( 6.2831f*587.3f * (float)i/(float)MZK_RATE );
	
		    //buffer[2*i+0] = (int)(fl*32767.0f);
			//buffer[2*i+1] = (int)(fr*32767.0f);
		}
		
		// Go to next instrument
		firstFrame += numFrames;
	}

	// Clear buffer:
	for (int i = 0; i < MZK_NUMSAMPLESC; i++)
	{
		buffer[i] = 0;
	}

	// Add sample 1:
	int bufferPos = 0;
	for (int samp = 0; samp < numInstruments; samp++)
	{
		for (int i = 0; i < sampleFrames[samp]*stepSize; i++)
		{
			buffer[bufferPos++] += (int)(sample[samp][i]);
			buffer[bufferPos++] += (int)(sample[samp][i]);
		}
	}
}
