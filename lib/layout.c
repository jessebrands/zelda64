//
// Created by Jesse on 13/08/2026.
//

#include "layout.h"

void zelda64_free_layout(struct zelda64_dmadata_layout* layout) {
    zelda64_free(layout->allocator, layout);
}
