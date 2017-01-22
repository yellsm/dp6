#include "terasic_os_includes.h"
#include "terasic_lib.h"


// link: -lrt

long get_tick_count(void)
{
    struct timespec now;
    clock_gettime(1, &now);
    return now.tv_sec*1000000 + now.tv_nsec/1000;
 
}
