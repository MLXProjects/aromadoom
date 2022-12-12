//doomgeneric for soso os

#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"

#include <stdio.h>
#include <unistd.h>

#include <stdbool.h>
#include <aroma.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct {
	int width;
	int height;
	
	byte smooth;
} DG_AromaConfig, * DG_AromaConfigP;

DG_AromaConfig aromacfg={0};
LIBAROMA_ZIP aromazip = NULL;
LIBAROMA_WINDOWP window = NULL;
LIBAROMA_CANVASP texture = NULL;
pthread_t input_poller_var;
pthread_mutex_t texture_mutex;
byte onmenu=2;

#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

static unsigned char convertToDoomKey(LIBAROMA_MSGP msg){
  unsigned int key;
  switch (msg->key)
    {
    /*case SDLK_RETURN:
      key = KEY_ENTER;
      break;
    case SDLK_ESCAPE:
      key = KEY_ESCAPE;
      break;
    case SDLK_LEFT:
      key = KEY_LEFTARROW;
      break;
    case SDLK_RIGHT:
      key = KEY_RIGHTARROW;
      break;
    case SDLK_UP:
      key = KEY_UPARROW;
      break;
    case SDLK_DOWN:
      key = KEY_DOWNARROW;
      break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
      key = KEY_FIRE;
      break;
    case SDLK_SPACE:
      key = KEY_USE;
      break;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
      key = KEY_RSHIFT;
      break;*/
    }

  return key;
}

static void addKeyToQueue(int pressed, unsigned char key){
  //unsigned char key = convertToDoomKey(keyCode);

  unsigned short keyData = (pressed << 8) | key;

  s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
  s_KeyQueueWriteIndex++;
  s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

void input_poller(void *data){
	window->onpool=1;
	LIBAROMA_MSG msg;
	dword command;
	do {
		command = libaroma_window_pool(window, &msg);
		if (msg.msg==LIBAROMA_MSG_EXIT){
			printf("InputPoller: requested to exit\n");
			window->onpool=0;
			exit(1);
		}
		else if (msg.msg==LIBAROMA_MSG_KEY_VOLUP){
			printf("InputPoller: VOL UP\n");
			addKeyToQueue(msg.state, (onmenu)?KEY_UPARROW:KEY_FIRE);
		}
		else if (msg.msg==LIBAROMA_MSG_KEY_VOLDOWN){
			printf("InputPoller: VOL DOWN\n");
			addKeyToQueue(msg.state, (onmenu)?KEY_DOWNARROW:KEY_RSHIFT);
		}
		else if (msg.msg==LIBAROMA_MSG_KEY_POWER){
			printf("InputPoller: POWER\n");
			if (onmenu>0){
				if (msg.state==0) addKeyToQueue(msg.state, KEY_FIRE);
			}
			else {
				window->onpool=0;
				exit(1);
			}
			onmenu--;
		}
		else if (LIBAROMA_MSG_ISUSER(msg.msg)){
			addKeyToQueue(msg.state, msg.key);
		}
		else if (LIBAROMA_CMD(command)==LIBAROMA_CMD_CLICK){
			printf("InputPoller: id=%d\n", LIBAROMA_CMD_ID(command));
			libaroma_msg_post(LIBAROMA_MSG_USR(0), 1, LIBAROMA_CMD_ID(command), 0, 0, NULL);
			libaroma_msg_post(LIBAROMA_MSG_USR(0), 0, LIBAROMA_CMD_ID(command), 0, 0, NULL);
		}
	} while(window->onpool);
}

void scrbtn_onclick(LIBAROMA_CONTROLP ctl){
	libaroma_png_save(texture, "/tmp/doom.png");
}

void closebtn_onclick(LIBAROMA_CONTROLP ctl){
	libaroma_msg_post(LIBAROMA_MSG_EXIT,0,0,0,0,NULL);
}

void DG_AtExit(){
  if (window!=NULL) {
	libaroma_wm_set_active_window(NULL);
	libaroma_window_free(window);
  }
  libaroma_end();
  if (texture!=NULL){
	libaroma_canvas_free(texture);
  }
  if (aromazip!=NULL){
	libaroma_zip_release(aromazip);
  }
  pthread_mutex_lock(&texture_mutex);
  pthread_mutex_unlock(&texture_mutex);
  pthread_mutex_destroy(&texture_mutex);
}

void DG_Init(){
  printf("doomgeneric libaroma port by MLX\n");
  atexit(&DG_AtExit);
  if (myargc>3){
	printf("DG_Init detected running from ZIP=%s\n", myargv[3]);
	aromazip = libaroma_zip(myargv[3]);
	if (!aromazip){
		printf("DG_Init failed to open zip\n");
		libaroma_end();
		exit(1);
	}
	printf("DG_Init extracting DOOM.WAD from zip\n");
	if (!libaroma_zip_extract(aromazip, "DOOM.WAD", "/tmp/DOOM.WAD")){
		printf("DG_Init failed to extract DOOM.WAD to /tmp\n");
		libaroma_zip_release(aromazip);
		libaroma_end();
		exit(1);
	}
	if (atoi(myargv[2])!=0) libaroma_config()->runtime_monitor=LIBAROMA_START_MUTEPARENT;
  }
  printf("DG_Init starting libaroma\n");
  if (!libaroma_start()) {
	  printf("DG_Init libaroma start failed\n");
	  exit(1);
  }
  if (aromazip!=NULL){
    printf("DG_Init load font - id=0\n");
    libaroma_font(0,
  	  libaroma_stream_mzip(
	    aromazip, "Roboto-Regular.ttf"
	  )
    );
  }
  printf("DG_Init creating window\n");
  window = libaroma_window(NULL, 0, LIBAROMA_POS_HALF, LIBAROMA_SIZE_FULL, LIBAROMA_SIZE_HALF);
  if (window==NULL) {
	  printf("DG_Init window create failed\n");
	  exit(1);
  }
  printf("DG_Init creating canvas\n");
  texture = libaroma_canvas(DOOMGENERIC_RESX, DOOMGENERIC_RESY);
  if (texture==NULL) {
	  printf("DG_Init canvas create failed\n");
	  exit(1);
  }
  aromacfg.width=libaroma_fb()->w;
  aromacfg.height=libaroma_fb()->h/2;
  printf("DG_Init init texture mutex\n");
  pthread_mutex_init(&texture_mutex, NULL);
  printf("DG_Init creating button\n");
  libaroma_ctl_button(window, KEY_UPARROW, 48, 0, 48, 48, "W", LIBAROMA_CTL_BUTTON_RAISED, 0);
  libaroma_ctl_button(window, KEY_LEFTARROW, 0, 48, 48, 48, "A", LIBAROMA_CTL_BUTTON_RAISED, 0);
  libaroma_ctl_button(window, KEY_DOWNARROW, 48, 48, 48, 48, "S", LIBAROMA_CTL_BUTTON_RAISED, 0);
  libaroma_ctl_button(window, KEY_RIGHTARROW, 96, 48, 48, 48, "D", LIBAROMA_CTL_BUTTON_RAISED, 0);
  libaroma_ctl_button(window, KEY_ESCAPE, 0, 96, 72, 48, "Esc", LIBAROMA_CTL_BUTTON_RAISED, 0);
  libaroma_ctl_button(window, KEY_USE, 96, 96, 72, 48, "Use", LIBAROMA_CTL_BUTTON_RAISED, 0);
  //libaroma_control_set_onclick(scrbtn, &scrbtn_onclick);
  //libaroma_control_set_onclick(closebtn, &closebtn_onclick);
  printf("DG_Init showing window\n");
  libaroma_window_show(window);
  printf("DG_Init create input poller thread\n");
  int thread_done = pthread_create(&input_poller_var, NULL, &input_poller, NULL);
  printf("DG_Init done=%d\n", thread_done);
}

void DG_DrawFrame()
{
  pthread_mutex_lock(&texture_mutex);
  int i;
  const dwordp src = (const dwordp)DG_ScreenBuffer;
  for (i = 0; i < DOOMGENERIC_RESX*DOOMGENERIC_RESY; i++) {
    texture->data[i] = libaroma_rgb_to16(src[i]);
  }
  //libaroma_draw(libaroma_fb()->canvas, texture, 0, libaroma_dp(48), 0);
  libaroma_draw_scale_nearest(libaroma_fb()->canvas, texture, 0, 0, aromacfg.width, aromacfg.height, 0, 0, texture->w, texture->h);
  pthread_mutex_unlock(&texture_mutex);
  libaroma_fb_sync_area(0, 0, aromacfg.width, aromacfg.height);
  //usleep (16 * 1000);
  DG_SleepMs(16);
  //handleKeyInput();
}

void DG_SleepMs(uint32_t ms)
{
    usleep (ms * 1000);
}

uint32_t DG_GetTicksMs()
{
    struct timeval  tp;
    struct timezone tzp;

    gettimeofday(&tp, &tzp);

    return (tp.tv_sec * 1000) + (tp.tv_usec / 1000); /* return milliseconds */
}

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
  if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex){
    //key queue is empty
    return 0;
  }else{
    unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
    s_KeyQueueReadIndex++;
    s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

    *pressed = keyData >> 8;
    *doomKey = keyData & 0xFF;

    return 1;
  }

  return 0;
}

void DG_SetWindowTitle(const char * title)
{/*
  if (window != NULL){
    SDL_SetWindowTitle(window, title);
  }*/
}
