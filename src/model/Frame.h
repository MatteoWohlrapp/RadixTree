#pragma once

#include "./Header.h"
#include <stdint.h>

struct Frame
{
    uint16_t fix_count = 0;
    bool dirty = false;
    bool marked = false; 
    Header header;
};
