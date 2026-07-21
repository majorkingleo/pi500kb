#include <pthread.h>

#ifndef HOOK_PATH
#define HOOK_PATH "/home/pi/pi500kb/hook.sh"
#endif

#include "keyqueue.h"

int initUSB();
void sendHIDReport();
