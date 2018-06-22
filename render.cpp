#include <Bela.h>
#include <cmath>
#include "drums.h"
#include "filter.h"

/* Drum samples are pre-loaded in these buffers. Length of each
 * buffer is given in gDrumSampleBufferLengths.
 */
extern float *gDrumSampleBuffers[NUMBER_OF_DRUMS];
extern int gDrumSampleBufferLengths[NUMBER_OF_DRUMS];

int gIsPlaying = 0;            /* Whether we should play or not. Implement this in Step 4b. */

/* Read pointer into the current drum sample buffer.
 */

int gReadPointer;
#define NUMBER_OF_READPOINTERS 16
int gReadPointers[NUMBER_OF_READPOINTERS];
int gDrumBufferForReadPointer[NUMBER_OF_READPOINTERS];
bool gReadPointersActive[NUMBER_OF_READPOINTERS];

/* Patterns indicate which drum(s) should play on which beat.
 * Each element of gPatterns is an array, whose length is given
 * by gPatternLengths.
 */
extern int *gPatterns[NUMBER_OF_PATTERNS];
extern int gPatternLengths[NUMBER_OF_PATTERNS];

/* These variables indicate which pattern we're playing, and
 * where within the pattern we currently are. Used in Step 4c.
 */
int gCurrentPattern = 0;
int gCurrentIndexInPattern = 0;
int gFillPattern = 5;


/* Triggers from buttons (step 2 etc.). Read these here and
 * do something if they are nonzero (resetting them when done). */
int gTriggerButton1;
int gTriggerButton2;

/* This variable holds the interval between events in **milliseconds**
 * To use it (Step 4a), you will need to work out how many samples
 * it corresponds to.
 */
int gEventIntervalMilliseconds = 250;

/* This variable indicates whether samples should be triggered or
 * not. It is used in Step 4b, and should be set in gpio.cpp.
 */
extern int gIsPlaying;

/* This indicates whether we should play the samples backwards.
 */
int gPlaysBackwards = 0;

/* For bonus step only: these variables help implement a fill
 * (temporary pattern) which is triggered by tapping the board.
 */
int gShouldPlayFill = 0;
int gPreviousPattern = 0;
int gSampleCount = 0;

// Varilables for button 1
int gButtonPin1= P8_07;
int gButtonState1 = 0;
int gButtonPreviousState1 = 0;

// Varilables for button 2
int gButtonPin2 = P8_08;
int gButtonState2 = 0;
int gButtonPreviousState2 = 0;

// Varilables for LED
int gLedPin = P8_09;
int gLedState = 0;

// Varilables for Potentiometer
int gPotPin = 0;
int gNumAudioFramesPerAnalog;

// Varilables for Accelerometer
int gAccelPinX = 1;
int gAccelPinY = 2;
int gAccelPinZ = 3;
int gAccelTrigger = 0;
float gAccelMean = 0.42;

// Enumeration to set the orientation states
enum {
    flat = 0,
    tiltLeft,
    tiltRight,
    tiltFront,
    tiltBack,
    upsideDown,
};

// Varilables for Orientation
int gOrientation;
float gXState = 0;
float gYState = 0;
float gZState = 0;

// Upper and Lower Hysterisis Threhsold while calculating orientation
float gHysterisisUpperThreshold = 0.07;
float gHysterisisLowerThreshold = 0.04;

// Varilables for High Pass Filter
filter gHPF;
float gCutoffFrequency = 150.0;
float gFilterThreshold = 0.5;

// Method to calculate the orientation of the Accelerometer and set the state
void calculateOrientation(float x, float y, float z)
{
    if(gXState == 0 && fabs(x) > gHysterisisUpperThreshold)
        gXState = x/fabs(x);
    else if (gXState != 0 && fabs(x) < gHysterisisLowerThreshold)
        gXState = 0;
    
    if(gYState == 0 && fabs(y) > gHysterisisUpperThreshold)
        gYState = y/fabs(y);
    else if (gYState != 0 && fabs(y) < gHysterisisLowerThreshold)
        gYState = 0;
    
    if(gZState == 0 && fabs(z) > gHysterisisUpperThreshold)
        gZState = z/fabs(z);
    else if (gZState != 0 && fabs(z) < gHysterisisLowerThreshold)
        gZState = 0;
    
    if (gXState == 0 && gYState == 0 && gZState == 1){
        gOrientation = flat;
        gPlaysBackwards = 0;
    }
    if (gXState == -1 && gYState == 0 && gZState == 0){
        gOrientation = tiltLeft;
        gPlaysBackwards = 0;
    }
    if (gXState == 1 && gYState == 0 && gZState == 0){
        gOrientation = tiltRight;
        gPlaysBackwards = 0;
    }
    if (gXState == 0 && gYState == -1 && gZState == 0){
        gOrientation = tiltFront;
        gPlaysBackwards = 0;
    }
    if (gXState == 0 && gYState == 1 && gZState == 0){
        gOrientation = tiltBack;
        gPlaysBackwards = 0;
    }
    if (gXState == 0 && gYState == 0 && gZState == -1){
        gPlaysBackwards = 1;
    }
}

// setup() is called once before the audio rendering starts.
// Use it to perform any initialisation and allocation which is dependent
// on the period size or sample rate.
//
// userData holds an opaque pointer to a data structure that was passed
// in from the call to initAudio().
//
// Return true on success; returning false halts the program.

bool setup(BelaContext *context, void *userData)
{
    /* Step 2: initialise GPIO pins */
    pinMode(context, 0, gButtonPin1, INPUT);
    pinMode(context, 0, gButtonPin2, INPUT);
    pinMode(context, 0, gLedPin, OUTPUT);
    gNumAudioFramesPerAnalog = context->audioFrames / context->analogFrames;
    
    for (int i = 0; i < NUMBER_OF_READPOINTERS; i++){
        gReadPointers[i] = 0;
        gDrumBufferForReadPointer[i] = -1;
    }
    
    gHPF.resetFilter();
    gHPF.getCoefficients(gCutoffFrequency, context->audioSampleRate);
    gHPF.setCoefficients(gHPF.getB(), gHPF.getA());
    gShouldPlayFill = 0;
    
    return true;
}

// render() is called regularly at the highest priority by the audio engine.
// Input and output are given from the audio hardware and the other
// ADCs and DACs (if available). If only audio is available, numMatrixFrames
// will be 0

void render(BelaContext *context, void *userData)
{	
	// Intialise the output
    float out[NUMBER_OF_READPOINTERS] = {0.0};

    for (unsigned int n = 0; n < context->audioFrames; n++) {

        // Read the buttons
        gButtonState1 = digitalRead(context, n, gButtonPin1);

        // Toggle the trigger when button is released
        if(gButtonState1 == 0 && gButtonPreviousState1 == 1){
            gTriggerButton1 = 1;
            gIsPlaying = !gIsPlaying;
            gSampleCount = 0;
        }
        gButtonPreviousState1 = gButtonState1;

        // Get the Potentiometer Value
        float potValue = analogRead(context, n/gNumAudioFramesPerAnalog, gPotPin);
        
        // Map the output value range of the potentiometer to between 50 and 1000ms
        gEventIntervalMilliseconds = map(potValue, 0, 0.829, 50, 1000);
        
        // If the system is playing trigger the LED State according to the tempo
        if(gIsPlaying) {
            if(gSampleCount >= gEventIntervalMilliseconds * 0.001 * context->audioSampleRate) {
                startNextEvent();
                gSampleCount = 0;
                if(gLedState == 0)
                    gLedState = 1;
                else
                    gLedState = 0;
            }
            gSampleCount++;
        }
        digitalWrite(context, n, gLedPin, gLedState);

        // Obtain the x, y & z axis values
        // Subtract the mean from it to obtain positive and negative values
        float x = analogRead(context, n/gNumAudioFramesPerAnalog, gAccelPinX) - gAccelMean;
        float y = analogRead(context, n/gNumAudioFramesPerAnalog, gAccelPinY) - gAccelMean;
        float z = analogRead(context, n/gNumAudioFramesPerAnalog, gAccelPinZ) - gAccelMean;

        // Applying the filter
        float filterInput = sqrt(pow(x,2) + pow(y,2) + pow(z,2));
        float filterOutput = gHPF.processFilter(filterInput);

        // Check if Filtered Output is greater than threshold and if no fill is being played
        // Play fill if condition matches
        if (filterOutput > gFilterThreshold && gShouldPlayFill == 0) {
            gShouldPlayFill = 1;
            if (gCurrentPattern != gFillPattern)
                gPreviousPattern = gCurrentPattern;
            gCurrentIndexInPattern = 0;
        }

        // Calculate Orientation for every 100 iterations.
        // Set the appropriate pattern to be played
        gAccelTrigger++;
        if(gAccelTrigger == 100)
        {
            gAccelTrigger = 0;
            calculateOrientation(x,y,z);
            if(!gShouldPlayFill)
                gCurrentPattern = gOrientation;
            else if (gShouldPlayFill)
                gCurrentPattern = gFillPattern;
            gCurrentIndexInPattern = gCurrentIndexInPattern % gPatternLengths[gCurrentPattern];
        }

        // For all the read pointers play the appropriate sample as per the sequency
        // If the orientation state is not playing backwards
        // Then the pointers loop from 0 to the Drum Sample Buffer Length
        // Else if the orientation state is to play backwards
        // Then the pointers loop backwards from Drum Sample Buffer Length to 0
        for (int i = 0; i < NUMBER_OF_READPOINTERS; i++) {
            if(gDrumBufferForReadPointer[i] >= 0){
                out[i] = gDrumSampleBuffers[gDrumBufferForReadPointer[i]][gReadPointers[i]];
            }
            if(!gPlaysBackwards) {
                gReadPointers[i]++;
                if(gReadPointers[i] >= gDrumSampleBufferLengths[gDrumBufferForReadPointer[i]]) {
                    gDrumBufferForReadPointer[i] = -1;
                    gTriggerButton1 = 0;
                }
            } else if (gPlaysBackwards) {
                gReadPointers[i]--;
                if(gReadPointers[i] <= 0) {
                    gDrumBufferForReadPointer[i] = -1;
                    gTriggerButton1 = 0;
                }
            }
        }

        // Consolidate all the out array values and scale it by the number of drums played
        for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
            for (int i = 0; i < NUMBER_OF_READPOINTERS; i++) {
                context->audioOut[n * context->audioOutChannels + channel] += out[i];
            }
            context->audioOut[n * context->audioOutChannels + channel] = context->audioOut[n * context->audioOutChannels + channel]/NUMBER_OF_READPOINTERS;
        }
    }
}

/* Start playing a particular drum sound given by drumIndex */
void startPlayingDrum(int drumIndex) {
    /* Read the pointer that is not already being used to play
     * use gDrumBufferForReadPointer to indicate which buffer should be played
     * if there are no read pointers free; return without playing the sound.
     */
    for (int i = 0; i < NUMBER_OF_READPOINTERS; i++){
        if(!(gDrumBufferForReadPointer[i] >= 0)){
            // The sample which will be played
            gDrumBufferForReadPointer[i] = drumIndex;
            
            /* If the orientation state is not playing backwards
             * Then the pointers reset to 0
             * Else if the orientation state is to play backwards
             * Then the pointers reset to  Drum Sample Buffer Length - 1
             */
            if(!gPlaysBackwards)
                gReadPointers[i] = 0;
            if(gPlaysBackwards)
                gReadPointers[i] = gDrumSampleBufferLengths[i] - 1;
            
            break;
        }
    }
}

/* Start playing the next event in the pattern */
void startNextEvent() {
    int event = gPatterns[gCurrentPattern][gCurrentIndexInPattern];
    rt_printf("Pattern: %d\t Index: %d\n", gCurrentPattern, gCurrentIndexInPattern);

    // Check if the event contains drum and trigger the read pointers to go through the sample
    for(int i = 0; i< NUMBER_OF_DRUMS; i++) {
        if(eventContainsDrum(event,i))
            startPlayingDrum(i);
    }
    gCurrentIndexInPattern++;

    // If the pattern index goes out of bounds
    // Restart the drum sequeunce
    // Unless a fill is being played
    if(gCurrentIndexInPattern >= gPatternLengths[gCurrentPattern]) {
        gCurrentIndexInPattern = 0;
        if (gCurrentPattern == gFillPattern){
            gCurrentPattern = gPreviousPattern;
            gShouldPlayFill = 0;
        }
    }
}

/* Returns whether the given event contains the given drum sound */
int eventContainsDrum(int event, int drum) {
    if(event & (1 << drum))
        return 1;
    return 0;
}

// cleanup_render() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in initialise_render().
void cleanup(BelaContext *context, void *userData)
{
    
}

