#ifndef UTILS_H_
#define UTILS_H_
#define LOG_TAG "RIL_TEST"
#include <utils/Log.h>
#define ASSERT(p) \
	if(!(p)) \
    { \
    	LOGE("%s:%d !!!ASSERTION failed: "#p" failed.\n", \
            __func__, __LINE__); \
    }
    /*
    else \
    { \
    	LOGE("%s:%d check "#p" success.\n", \
            __func__, __LINE__); \
    }*/

#define MAX_BUFFER_SIZE 256
#define REGISTER_HOME 1
#define REGISTER_ROAMING 5
#define exit_if_fail(p) if(!(p)) {LOGE(""#p" failed.\n"); exit(-1);}
#define return_val_if_fail(p, ret) if(!(p)) \
	{LOGE("%s:%d Warning: "#p" failed.\n",\
	__func__, __LINE__); return (ret);}

/*
 * return 0x02 when success
 * return 0x00 when timeout
 * return negative number when error happens.
 *
 * */
int my_read_select (int read_fd, long timeout);

#endif
