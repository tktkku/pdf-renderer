#ifndef PDF_CMAPS_H
#define PDF_CMAPS_H

#include "pdf-private.h"

// Global CMAP index (sorted by name)
typedef struct {
    const char *name;
    pdf_cmap_t *cmap;
} CMAP_ENTRY;
extern const CMAP_ENTRY g_CMAP_INDEX[189];

#endif // #ifndef PDF_CMAPS_H

