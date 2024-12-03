#ifndef TASKCOMMONS_H
#define TASKCOMMONS_H

/*-------------------------------------------- WIFI TASK--------------------------------*/
#define WIFI_TASK_STACK       8192
#define WIFI_TASK_PRIORITY     3
#define WIFI_TASK_CORE         0

/*-------------------------------------------- SMARTCONFIG_TASK-------------------------------*/
#define SMARTCONFIG_TASK_STACK      4096
#define SMARTCONFIG_TASK_PRIORITY     (WIFI_TASK_PRIORITY - 1)
#define SMARTCONFIG_TASK_CORE         1



#endif ///> TASKCOMMONS_H