#include <stdint.h>
#pragma once

struct Header
{
    uint32_t page_id;
    // specifies if inner or outer node
    bool inner = false;
    char padding[3];

    Header(uint32_t page_id_arg, bool inner_arg) : page_id(page_id_arg), inner(inner_arg){};

    Header(Header *other) : page_id(other->page_id), inner(other->inner) {}
};