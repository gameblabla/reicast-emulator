#include "oslib/audiobackend_portaudio.h"

#ifdef USE_PORTAUDIO
#include <portaudio.h>
#include <unistd.h>

static PaStream *apu_stream;

static void portaudio_init()
{
	int32_t err;
	err = Pa_Initialize();
	
	PaStreamParameters outputParameters;
	
	outputParameters.device = Pa_GetDefaultOutputDevice();
	
	if (outputParameters.device == paNoDevice) 
	{
		return EXIT_FAILURE;
	}
	
    err = Pa_OpenDefaultStream( &apu_stream, 0, 2, paInt16, 44100, 1024, nullptr, nullptr );
	err = Pa_StartStream( apu_stream );
}

static u32 portaudio_push(void* frame, u32 samples, bool wait)
{
	if (apu_stream)
	{
		Pa_WriteStream( apu_stream, frame, samples );
	}
	
	return 1;
}

static void portaudio_term() 
{
	int32_t err;
	if (apu_stream)
	{
		err = Pa_CloseStream( apu_stream );
		err = Pa_Terminate();
	}
}

audiobackend_t audiobackend_portaudio = {
		"portaudio", // Slug
		"PortAudio", // Name
		&portaudio_init,
		&portaudio_push,
		&portaudio_term
};

#endif
