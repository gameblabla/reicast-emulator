#if defined(USE_SDL)

#include <unistd.h>
#include <poll.h>
#include <termios.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/time.h>


#include "types.h"
#include "cfg/cfg.h"
#include "linux-dist/main.h"
#include "sdl/sdl.h"
#ifndef GLES
#include "khronos/GL3/gl3w.h"
#endif
#endif

static SDL_Window* window = NULL;
static SDL_GLContext glcontext;

#ifdef TARGET_PANDORA
	#define WINDOW_WIDTH  800
#else
	#define WINDOW_WIDTH  640
#endif
#define WINDOW_HEIGHT  480

static SDL_Joystick* JoySDL = 0;

extern bool FrameSkipping;
extern void dc_stop();
extern bool KillTex;

#ifdef TARGET_PANDORA
	extern char OSD_Info[128];
	extern int OSD_Delay;
	extern char OSD_Counters[256];
	extern int OSD_Counter;
#endif

#define SDL_MAP_SIZE 32

const u32 sdl_map_btn_usb[SDL_MAP_SIZE] =
	{ DC_BTN_Y, DC_BTN_B, DC_BTN_A, DC_BTN_X, 0, 0, 0, 0, 0, DC_BTN_START };

const u32 sdl_map_axis_usb[SDL_MAP_SIZE] =
	{ DC_AXIS_X, DC_AXIS_Y, 0, 0, 0, 0, 0, 0, 0, 0 };

const u32 sdl_map_btn_xbox360[SDL_MAP_SIZE] =
	{ DC_BTN_A, DC_BTN_B, DC_BTN_X, DC_BTN_Y, 0, 0, 0, DC_BTN_START, 0, 0 };

const u32 sdl_map_axis_xbox360[SDL_MAP_SIZE] =
	{ DC_AXIS_X, DC_AXIS_Y, DC_AXIS_LT, 0, 0, DC_AXIS_RT, DC_DPAD_LEFT, DC_DPAD_UP, 0, 0 };

const u32* sdl_map_btn  = sdl_map_btn_usb;
const u32* sdl_map_axis = sdl_map_axis_usb;

#ifdef TARGET_PANDORA
u32  JSensitivity[256];  // To have less sensitive value on nubs
#endif

u16 kcode[4];
extern u32 vks[4];
u8 rt[4], lt[4];
s8 joyx[4], joyy[4];

int msgboxf(const wchar* text, unsigned int type, ...)
{
	va_list args;

	wchar temp[2048];
	va_start(args, type);
	vsprintf(temp, text, args);
	va_end(args);

	//printf(NULL,temp,VER_SHORTNAME,type | MB_TASKMODAL);
	puts(temp);
	return MBX_OK;
}

void os_DebugBreak()
{
}

void* x11_win = 0;
void* x11_disp = 0;
void* x11_glc = 0;

void* libPvr_GetRenderTarget()
{
	return x11_win;
}

void* libPvr_GetRenderSurface()
{
	return x11_disp;
}

int get_mic_data(u8* buffer) { return 0; }
int push_vmu_screen(u8* buffer) { return 0; }
void UpdateVibration(u32 port, u32 value) { }
void common_linux_setup();
int dc_init(int argc,wchar* argv[]);
void dc_run();
void dc_term();


void os_SetupInput()
{
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == 0)
	{
		if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
		{
			die("error initializing SDL Joystick subsystem");
		}
	}
	// Open joystick device
	int numjoys = SDL_NumJoysticks();
	printf("Number of Joysticks found = %i\n", numjoys);
	if (numjoys > 0)
	{
		JoySDL = SDL_JoystickOpen(0);
	}
	
	printf("Joystick opened\n");  
	
	if(JoySDL)
	{
		int AxisCount,ButtonCount;
		const char* Name;

		AxisCount   = 0;
		ButtonCount = 0;
		//Name[0]     = '\0';

		AxisCount = SDL_JoystickNumAxes(JoySDL);
		ButtonCount = SDL_JoystickNumButtons(JoySDL);
		Name = SDL_JoystickName(JoySDL);

		printf("SDK: Found '%s' joystick with %d axes and %d buttons\n", Name, AxisCount, ButtonCount);

		if (Name != NULL && strcmp(Name,"Microsoft X-Box 360 pad")==0)
		{
			sdl_map_btn  = sdl_map_btn_xbox360;
			sdl_map_axis = sdl_map_axis_xbox360;
			printf("Using Xbox 360 map\n");
		}
	}
	else
	{
		printf("SDK: No Joystick Found\n");
	}

	#ifdef TARGET_PANDORA
		float v;
		int j;
		for (int i=0; i<128; i++)
		{
			v = ((float)i)/127.0f;
			v = (v+v*v)/2.0f;
			j = (int)(v*127.0f);
			if (j > 127)
			{
				j = 127;
			}
			JSensitivity[128-i] = -j;
			JSensitivity[128+i] = j;
		}
	#endif
	
	SDL_SetRelativeMouseMode(SDL_TRUE);
}

void UpdateInputState(u32 port)
{
	static int keys[13];
	static int mouse_use = 0;
	SDL_Event event;
	int k, value;
	int xx, yy;
	const char *num_mode[] = {"Off", "Up/Down => RT/LT", "Left/Right => LT/RT", "U/D/L/R => A/B/X/Y"};
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
							 die("death by SDL request");
							 break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				k = event.key.keysym.sym;
				value = (event.type == SDL_KEYDOWN) ? 1 : 0;
				 //printf("type %i key %i \n", event.type, k);
				switch (k) {
					//TODO: Better keymaps for non-pandora platforms
					case SDLK_SPACE:    keys[0] = value; break;
					case SDLK_UP:       keys[1] = value; break;
					case SDLK_DOWN:     keys[2] = value; break;
					case SDLK_LEFT:     keys[3] = value; break;
					case SDLK_RIGHT:    keys[4] = value; break;
					case SDLK_PAGEUP:   keys[5] = value; break;
					case SDLK_PAGEDOWN: keys[6] = value; break;
					case SDLK_END:      keys[7] = value; break;
					case SDLK_HOME:     keys[8] = value; break;
					case SDLK_MENU:
					case SDLK_ESCAPE:   keys[9]  = value; break;
					case SDLK_RSHIFT:   keys[11] = value; break;
					case SDLK_RCTRL:    keys[10] = value; break;
					case SDLK_LALT:     keys[12] = value; break;
					case SDLK_k:        KillTex = true; break;
				}
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				value = (event.type == SDL_JOYBUTTONDOWN) ? 1 : 0;
				k = event.jbutton.button;
				{
					u32 mt = sdl_map_btn[k] >> 16;
					u32 mo = sdl_map_btn[k] & 0xFFFF;
					
					// printf("BUTTON %d,%d\n",JE.number,JE.value);
					
					if (mt == 0)
					{
						// printf("Mapped to %d\n",mo);
						if (value)
						kcode[port] &= ~mo;
						else
						kcode[port] |= mo;
					}
					else if (mt == 1)
					{
						// printf("Mapped to %d %d\n",mo,JE.value?255:0);
						if (mo == 0)
						{
							lt[port] = value ? 255 : 0;
						}
						else if (mo == 1)
						{
							rt[port] = value ? 255 : 0;
						}
					}
					
				}
				break;
			case SDL_JOYAXISMOTION:
				k = event.jaxis.axis;
				value = event.jaxis.value;
				{
					u32 mt = sdl_map_axis[k] >> 16;
					u32 mo = sdl_map_axis[k] & 0xFFFF;
					
					//printf("AXIS %d,%d\n",JE.number,JE.value);
					s8 v=(s8)(value/256); //-127 ... + 127 range
					if (mt == 0)
					{
						kcode[port] |= mo;
						kcode[port] |= mo*2;
						if (v < -64)
						{
							kcode[port] &= ~mo;
						}
						else if (v > 64)
						{
							kcode[port] &= ~(mo*2);
						}
						
						// printf("Mapped to %d %d %d\n",mo,kcode[port]&mo,kcode[port]&(mo*2));
					}
					else if (mt == 1)
					{
						if (v >= 0) v++;  //up to 255
						
						//   printf("AXIS %d,%d Mapped to %d %d %d\n",JE.number,JE.value,mo,v,v+127);
						
						if (mo == 0)
						{
							lt[port] = v + 127;
						}
						else if (mo == 1)
						{
							rt[port] = v + 127;
						}
					}
					else if (mt == 2)
					{
						//  printf("AXIS %d,%d Mapped to %d %d [%d]",JE.number,JE.value,mo,v);
						if (mo == 0)
						{
							joyx[port] = v;
						}
						else if (mo==1)
						{
							joyy[port] = v;
						}
					}
				}
				break;
			case SDL_MOUSEMOTION:
					xx = event.motion.xrel;
					yy = event.motion.yrel;
					
					// some caping and dead zone...
					if (abs(xx) < 4)
					{
						xx = 0;
					}
					if (abs(yy) < 4)
					{
						yy = 0;
					}
					
					xx = xx * 255 / 20;
					yy = yy * 255 / 20;
					
					if (xx > 255)
					{
						xx = 255;
					}
					else if (xx<-255)
					{
						xx = -255;
					}
					
					if (yy > 255)
					{
						yy = 255;
					}
					else if (yy<-255)
					{
						yy = -255;
					}
					
					//if (abs(xx)>0 || abs(yy)>0) printf("mouse %i, %i\n", xx, yy);
					switch (mouse_use)
					{
						case 0:  // nothing
							break;
						case 1:  // Up=RT, Down=LT
							if (yy<0)
							{
								rt[port] = -yy;
							}
							else if (yy>0)
							{
								lt[port] = yy;
							}
							break;
						case 2:  // Left=LT, Right=RT
							if (xx < 0)
							{
								lt[port] = -xx;
							}
							else if (xx > 0)
							{
								rt[port] = xx;
							}
							break;
						case 3:  // Nub = ABXY
							if (xx < -127)
							{
								kcode[port] &= ~DC_BTN_X;
							}
							else if (xx > 127)
							{
								kcode[port] &= ~DC_BTN_B;
							}
							if (yy < -127)
							{
								kcode[port] &= ~DC_BTN_Y;
							}
							else if (yy > 127)
							{
								kcode[port] &= ~DC_BTN_A;
							}
							break;
					}
				break;
		}
	}
			
	if (keys[0]) { kcode[port] &= ~DC_BTN_C; }
	if (keys[6]) { kcode[port] &= ~DC_BTN_A; }
	if (keys[7]) { kcode[port] &= ~DC_BTN_B; }
	if (keys[5]) { kcode[port] &= ~DC_BTN_Y; }
	if (keys[8]) { kcode[port] &= ~DC_BTN_X; }
	if (keys[1]) { kcode[port] &= ~DC_DPAD_UP; }
	if (keys[2]) { kcode[port] &= ~DC_DPAD_DOWN; }
	if (keys[3]) { kcode[port] &= ~DC_DPAD_LEFT; }
	if (keys[4]) { kcode[port] &= ~DC_DPAD_RIGHT; }
	if (keys[12]){ kcode[port] &= ~DC_BTN_START; }
	if (keys[9])
	{ 
		dc_stop();
	} 
	if (keys[10])
	{
		rt[port] = 255;
	}
	if (keys[11])
	{
		lt[port] = 255;
	}
}

void os_DoEvents()
{
}

string find_user_config_dir()
{
	// Unable to detect config dir, use the current folder
	return ".";
}

string find_user_data_dir()
{
	// Unable to detect config dir, use the current folder
	return ".";
}

std::vector<string> find_system_config_dirs()
{
	std::vector<string> dirs;
	if (getenv("XDG_CONFIG_DIRS") != NULL)
	{
		string s = (string)getenv("XDG_CONFIG_DIRS");

		string::size_type pos = 0;
		string::size_type n = s.find(":", pos);
		while(n != std::string::npos)
		{
			dirs.push_back(s.substr(pos, n-pos) + "/reicast");
			pos = n + 1;
			n = s.find(":", pos);
		}
		// Separator not found
		dirs.push_back(s.substr(pos) + "/reicast");
	}
	else
	{
		dirs.push_back("/etc/reicast"); // This isn't part of the XDG spec, but much more common than /etc/xdg/
		dirs.push_back("/etc/xdg/reicast");
	}
	return dirs;
}

std::vector<string> find_system_data_dirs()
{
	std::vector<string> dirs;
	if (getenv("XDG_DATA_DIRS") != NULL)
	{
		string s = (string)getenv("XDG_DATA_DIRS");

		string::size_type pos = 0;
		string::size_type n = s.find(":", pos);
		while(n != std::string::npos)
		{
			dirs.push_back(s.substr(pos, n-pos) + "/reicast");
			pos = n + 1;
			n = s.find(":", pos);
		}
		// Separator not found
		dirs.push_back(s.substr(pos) + "/reicast");
	}
	else
	{
		dirs.push_back("/usr/local/share/reicast");
		dirs.push_back("/usr/share/reicast");
	}
	return dirs;
}

int main(int argc, wchar* argv[])
{
	/* Set directories */
	set_user_config_dir(find_user_config_dir());
	set_user_data_dir(find_user_data_dir());
	std::vector<string> dirs;
	dirs = find_system_config_dirs();
	for(unsigned int i = 0; i < dirs.size(); i++)
	{
		add_system_data_dir(dirs[i]);
	}
	dirs = find_system_data_dirs();
	for(unsigned int i = 0; i < dirs.size(); i++)
	{
		add_system_data_dir(dirs[i]);
	}
	printf("Config dir is: %s\n", get_writable_config_path("/").c_str());
	printf("Data dir is:   %s\n", get_writable_data_path("/").c_str());

	#if defined(USE_SDL)
	if (SDL_Init(0) != 0)
	{
		die("SDL: Initialization failed!");
	}
	#endif

	common_linux_setup();

	settings.profile.run_counts=0;

	dc_init(argc,argv);

	dc_run();
	
	dc_term();
	return 0;
}

void os_SetWindowText(const char* text)
{
	if(window)
	{
		SDL_SetWindowTitle(window, text);    // *TODO*  Set Icon also...
	}
}

void os_CreateWindow()
{
	if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
	{
		if(SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
		{
			die("error initializing SDL Joystick subsystem");
		}
	}

	int window_width  = cfgLoadInt("x11","width", WINDOW_WIDTH);
	int window_height = cfgLoadInt("x11","height", WINDOW_HEIGHT);

	int flags = SDL_WINDOW_OPENGL;
	
	#ifdef GLES
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	#else
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	#endif

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	window = SDL_CreateWindow("Reicast Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,	window_width, window_height, flags);
	if (!window)
	{
		die("error creating SDL window");
	}

	glcontext = SDL_GL_CreateContext(window);
	if (!glcontext)
	{
		die("Error creating SDL GL context");
	}
	SDL_GL_MakeCurrent(window, NULL);

	printf("Created SDL Window (%ix%i) and GL Context successfully\n", window_width, window_height);
}

extern int screen_width, screen_height;

bool gl_init(void* wind, void* disp)
{
	SDL_GL_MakeCurrent(window, glcontext);
	return true;
	/*#ifdef GLES
		return true;
	#else
		return gl3wInit() != -1 && gl3wIsSupported(3, 1);
	#endif*/
}

void gl_swap()
{
	SDL_GL_SwapWindow(window);

	/* Check if drawable has been resized */
	int new_width, new_height;
	SDL_GL_GetDrawableSize(window, &new_width, &new_height);

	if (new_width != screen_width || new_height != screen_height)
	{
		screen_width = new_width;
		screen_height = new_height;
	}
}

void gl_term()
{
	SDL_GL_DeleteContext(glcontext);
}
