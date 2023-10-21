#ifndef D2D2AD64_49E0_4754_B951_D53AC56C1D04
#define D2D2AD64_49E0_4754_B951_D53AC56C1D04

#include "qapi/qapi-builtin-types.h"
#include "qemu/queue.h"

struct VgaInstance {
    void* vga_state;// VGACommonState
    QLIST_ENTRY(VgaInstance) next;
};

typedef QLIST_HEAD(,VgaInstance) VgaInstanceList;

#endif /* D2D2AD64_49E0_4754_B951_D53AC56C1D04 */
