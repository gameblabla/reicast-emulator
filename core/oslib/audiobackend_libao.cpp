#include "oslib/audiobackend_libao.h"
#ifdef USE_LIBAO

#include <unistd.h>
#include <ao/ao.h>

static ao_device *aodevice;
static ao_sample_format aoformat;

static void libao_init()
{
	ao_initialize();
	memset(&aoformat, 0, sizeof(aoformat));
	
	aoformat.bits = 16;
	aoformat.channels = 2;
	aoformat.rate = 44100;
	aoformat.byte_format = AO_FMT_LITTLE;
	
	aodevice = ao_open_live(ao_default_driver_id(), &aoformat, NULL); // Live output
	if (!aodevice)
	{
		printf("Could not open sound device, using null driver instead \n");
		aodevice = ao_open_live(ao_driver_id("null"), &aoformat, NULL);
	}
}

static u32 libao_push(void* frame, u32 samples, bool wait)
{
	ao_play(aodevice, (char*)frame, samples * 4);
	
	return 1;
}

static void libao_term() 
{
	if (aodevice)
	{
		ao_close(aodevice);
		ao_shutdown();
	}
}

audiobackend_t audiobackend_libao = {
		"libao", // Slug
		"libao", // Name
		&libao_init,
		&libao_push,
		&libao_term
};

#endif
