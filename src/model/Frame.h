#pragma once

#include "./Header.h"
#include <stdint.h>

struct Frame
{
    uint8_t fix_count = 0;
    bool dirty = false;
    Header header;
};
