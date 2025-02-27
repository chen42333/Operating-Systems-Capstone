#include "boot.h"
#include "utils.h"

void reset(int tick) {
    set32(PM_RSTC, PM_PASSWORD | 0x20); 
    set32(PM_WDOG, PM_PASSWORD | tick);
}

void cancel_reset() {
    set32(PM_RSTC, PM_PASSWORD | 0);
    set32(PM_WDOG, PM_PASSWORD | 0);
}