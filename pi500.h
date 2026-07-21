#include <pthread.h>

#ifndef HOOK_PATH
#define HOOK_PATH "/home/pi/pi500kb/hook.sh"
#endif

int initUSB();
void sendHIDReport();
