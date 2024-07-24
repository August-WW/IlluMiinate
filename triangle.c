#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <wiiuse/wpad.h>

#include <asndlib.h>
#include <mp3player.h>
#include "sample_mp3.h"

#define DI_BUTTONS_DOWN 0
#define DI_BUTTONS_HELD 1

#define resetscreen() printf("\x1b[2J")

struct timespec _wiilight_timeOn;
struct timespec _wiilight_timeOff;

int WIILIGHT_IsOn;
int WIILIGHT_Level;

void _wiilight_turn(int enable);
static void *_wiilight_loop(void *arg);
static vu32 *_wiilight_reg = (u32 *)0xCD0000C0;
lwp_t _wiilight_thread;

void WIILIGHT_Init();
void WIILIGHT_TurnOn();
int WIILIGHT_GetLevel();
int WIILIGHT_SetLevel(int level);

void WIILIGHT_Toggle();
void WIILIGHT_TurnOff();

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void init_audio() {
	ASND_Init();
	MP3Player_Init();
	
	MP3Player_PlayBuffer(sample_mp3, sample_mp3_size, NULL);
}

void WIILIGHT_Init()
{
    _wiilight_timeOn.tv_sec = (time_t)0;
    _wiilight_timeOff.tv_sec = (time_t)0;
    WIILIGHT_IsOn = 0;
    WIILIGHT_SetLevel(0);
}

int WIILIGHT_GetLevel()
{
    return WIILIGHT_Level;
}

int WIILIGHT_SetLevel(int level)
{
    long level_on;
    long level_off;
    if (level > 255)
        level = 255;
    if (level < 0)
        level = 0;
    level_on = level * 40000;
    level_off = 10200000 - level_on;
    _wiilight_timeOn.tv_nsec = level_on;
    _wiilight_timeOff.tv_nsec = level_off;
    WIILIGHT_Level = level;
    return level;
}

void _wiilight_turn(int enable)
{
    u32 val = (*_wiilight_reg & ~0x20);
    if (enable)
        val |= 0x20;
    *_wiilight_reg = val;
}

void WIILIGHT_TurnOff()
{
    WIILIGHT_IsOn = false;
}

void WIILIGHT_TurnOn()
{
    WIILIGHT_IsOn = true;
    LWP_CreateThread(&_wiilight_thread, _wiilight_loop, NULL, NULL, 0, 80);
}

void WIILIGHT_Toggle()
{
    if (WIILIGHT_IsOn)
    {
        WIILIGHT_TurnOff();
    }
    else
    {
        WIILIGHT_TurnOn();
    }
}

static void *_wiilight_loop(void *arg)
{
    struct timespec timeOn;
    struct timespec timeOff;
    while (WIILIGHT_IsOn)
    {
        timeOn = _wiilight_timeOn;
        timeOff = _wiilight_timeOff;
        _wiilight_turn(1);
        nanosleep(&timeOn, &timeOn);
        if (timeOff.tv_nsec > 0)
            _wiilight_turn(0);
        nanosleep(&timeOff, &timeOff);
    }

    return NULL;
}

u32 DetectInput(u8 DownOrHeld)
{
    u32 pressed = 0;
    VIDEO_WaitVSync();

    if (WPAD_ScanPads() > WPAD_ERR_NONE)
    {
        if (DownOrHeld == DI_BUTTONS_DOWN)
        {
            pressed = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);
        }
        else
        {
            pressed = WPAD_ButtonsHeld(0) | WPAD_ButtonsHeld(1) | WPAD_ButtonsHeld(2) | WPAD_ButtonsHeld(3);
        }
    }

    return pressed;
}

int main(int argc, char **argv)
{
    VIDEO_Init();
    WPAD_Init();
	
	ASND_Init();
	MP3Player_Init();
	
	MP3Player_PlayBuffer(sample_mp3, sample_mp3_size, NULL);
	
    rmode = VIDEO_GetPreferredMode(NULL);

    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    WIILIGHT_Init();

        printf("\x1B[%d;%dH", 3, 0); 
		printf("\x1b[36;1m"); // Cyan text color
		printf("IlluMiinate - Light up the room with your Wii's disc drive!\n");
		printf("===========================================================\n");
		printf("Made by August Wolf - Press HOME to exit");
		printf("\x1B[%d;%dH", 8, 0); 
        printf("Press A to turn on the disc drive light!\n");

    while (1)
    {
        u32 pressed = DetectInput(DI_BUTTONS_DOWN);

        if (pressed & WPAD_BUTTON_A)
        {
            WIILIGHT_SetLevel(255);
            WIILIGHT_TurnOn();
        }

        if (pressed & WPAD_BUTTON_B)
        {
			WIILIGHT_Toggle();
			WIILIGHT_TurnOff();
        }

        if (pressed & WPAD_BUTTON_HOME)
        {
            printf("\nBye!");
            break;
        }

    }

    return 0;
}
