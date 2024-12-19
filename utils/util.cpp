#include "util.h"
int32_t find_pre_start_code(uint8_t *buf, int32_t bufLen) {
    int32_t i = 0;
    while (i < bufLen - 3) {
        if (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 0 &&
            buf[i + 3] == 1) {
            return i;
            break;
        }
        i++;
    }
    return -1;
}