#include <stdint.h>

#include "./Header.h"

#ifndef FRAME 
#define FRAME
struct Frame{
    uint8_t fix_count = 0; 
    bool dirty = false; 
    Header header; 
}; 
#endif