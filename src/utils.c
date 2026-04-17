#include <time.h>
#include "utils.h"

void get_current_time_str(char* buffer, size_t size){
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", local);
}