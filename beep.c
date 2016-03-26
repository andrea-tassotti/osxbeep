/**
 *
 *	beep
 *
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    (C) 2016 Andrea Tassotti
 *
 *	COMPILE WITH:
 *		gcc -framework Foundation -framework CoreAudio -framework AudioUnit -o beep beep.c
 *
 */
#include <AudioToolbox/AudioToolbox.h>
#include <unistd.h>


#pragma mark Structure and definitions

#define USEC_PER_CSEC 10000


/**
 */
typedef struct SineWavePlayer
{
	AudioUnit outputUnit;
	double startingFrameCount;
	float freq;
} SineWavePlayer;

OSStatus SineWaveRenderProc(void *inRefCon,
							AudioUnitRenderActionFlags *ioActionFlags,
							const AudioTimeStamp *inTimeStamp,
							UInt32 inBusNumber,
							UInt32 inNumberFrames,
							AudioBufferList * ioData);
void CreateAndConnectOutputUnit (SineWavePlayer *player) ;

#pragma mark Callback
/**
 */
OSStatus SineWaveRenderProc(void *inRefCon,
							AudioUnitRenderActionFlags *ioActionFlags,
							const AudioTimeStamp *inTimeStamp,
							UInt32 inBusNumber,
							UInt32 inNumberFrames,
							AudioBufferList * ioData)
{
	SineWavePlayer * player = (SineWavePlayer*)inRefCon;
	
	double j = player->startingFrameCount;
	double cycleLength = 44100. / player->freq;

	int frame = 0;
	for (frame = 0; frame < inNumberFrames; ++frame) 
	{
		((Float32 *)ioData->mBuffers[0].mData)[frame] =
		((Float32 *)ioData->mBuffers[1].mData)[frame] = (Float32)sin (2 * M_PI * (j / cycleLength));
		
		j += 1.0;
		if (j > cycleLength)
			j -= cycleLength;
	}
	
	player->startingFrameCount = j;
	return noErr;
}	


#pragma mark - Utility

/**
 * Error handler
 */
static void exitOnError(OSStatus err, const char *operation)
{
	if(err != noErr)
    {
        char formatID[5];
        *(UInt32 *)formatID = CFSwapInt32HostToBig(err);
        formatID[4] = '\0';
        fprintf(stderr, "ERROR: %s: %d '%-4.4s'\n", operation, (int)err, formatID);
        exit(1);
    }
}



/**
 *
 */
void CreateAndConnectOutputUnit (SineWavePlayer *player) {
	
	//  Query for Internal Output AudioUnit (10.6 and later)
	AudioComponentDescription outputcd = {0}; // 10.6 version
	outputcd.componentType = kAudioUnitType_Output;
	outputcd.componentSubType = kAudioUnitSubType_DefaultOutput;
	outputcd.componentManufacturer = kAudioUnitManufacturer_Apple;
	
	AudioComponent component = AudioComponentFindNext (NULL, &outputcd);
	if (component == NULL) {
		fprintf(stderr, "Can't get output unit");
		exit (-1);
	}
	exitOnError(AudioComponentInstanceNew(component, &player->outputUnit),
				"Couldn't open component for outputUnit");
	
	// Register render callback
	AURenderCallbackStruct input;
	input.inputProc = SineWaveRenderProc;
	input.inputProcRefCon = player;
	exitOnError(AudioUnitSetProperty(player->outputUnit,
									kAudioUnitProperty_SetRenderCallback, 
									kAudioUnitScope_Input,
									0,
									&input, 
									sizeof(input)),
			   "AudioUnitSetProperty failed");
		
	// Initialization
	exitOnError (AudioUnitInitialize(player->outputUnit),
				"Couldn't initialize output unit");
	
}

#pragma mark main

/**
 */
int	main(int argc, char * const argv[])
{
 	SineWavePlayer player = {0};

	int len = 10, ch;
	float freq = 750.0;	// Standard 'beep' frequency
	
	while ((ch = getopt(argc, argv, "d:lp:v?h")) != -1) {
		switch (ch) {
			// TODO: -l list AudioUnit Outputs available, with index
			//		 -d open AudioUnit Outputby id
			case 'd':
			case 'l':
				printf("Not implemented yet!\n");
				exit(0);
				break;
			case 'v':
				printf("beep for OSX - Version 1.0 (C)2016 Andrea Tassotti\n");
				return 0;
				break;
			case 'p':
				freq=atof(optarg);
				break;
			case '?':
			case 'h':
			default:
				printf("beep [-v] [-d device] [-p pitch] [-l] [duration]\n");
				return 1;
		}
	}
	
	argc -= optind;
	argv += optind;

	if (argc > 0)
		len = atof(*argv);

	// Setup CoreAudio
	CreateAndConnectOutputUnit(&player);
	player.freq = freq;

	// Start playing
	exitOnError(AudioOutputUnitStart(player.outputUnit), "Couldn't start output unit");
	
	// Duration
	usleep(USEC_PER_CSEC * len);
	
	// Stop playing
	AudioOutputUnitStop(player.outputUnit);
	
	// Cleanup
	AudioUnitUninitialize(player.outputUnit);
	AudioComponentInstanceDispose(player.outputUnit);
	
	return 0;
}
