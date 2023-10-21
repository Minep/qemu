#include "qemu/osdep.h"
#include "vga_int.h"
#include "vga-qmp.h"
#include "qapi/qapi-commands-vga.h"

#include "monitor/hmp.h"
#include "monitor/monitor.h"

extern VgaInstanceList vga_instances;

static VgaCrtInfo* vga_qmp_query_crt(VGACommonState* s) {
    VgaCrtInfo* crt = g_malloc(sizeof(VgaCrtInfo));
    
    s->update_retrace_info(s);

    struct vga_precise_retrace* crt_p = &s->retrace_info.precise;
    memcpy(crt, crt_p, sizeof(*crt_p));

    int w,h;
    s->get_resolution(s, &w, &h);
    crt->width = w;
    crt->height = h;

    return crt;
}

#define GMODE_TEXT     0
#define GMODE_GRAPH    1
#define GMODE_BLANK 2

static VgaGfxInfo* vga_qmp_query_gfx(VGACommonState* s) {
    VgaGfxInfo* gfx = g_malloc(sizeof(VgaCrtInfo));

    gfx->mode = (char*)g_malloc(16);
    switch (s->graphic_mode)
    {
    case GMODE_TEXT:
        strcpy(gfx->mode, "Text");
        break;
    case GMODE_GRAPH:
        strcpy(gfx->mode, "Graphic");
        break;
    case GMODE_BLANK:
        strcpy(gfx->mode, "Blank");
        break;
    default:
        strcpy(gfx->mode, "None");
        break;
    }

    return gfx;
}

static VgaRegisterInfo* vga_qmp_dump_regs(VGACommonState* s) {
    VgaRegisterInfo* reginfo = g_malloc(sizeof(VgaRegisterInfo));

    memcpy(&reginfo->sr0, s->sr, sizeof(0x5 * sizeof(uint8_t)));
    memcpy(&reginfo->ar0, s->ar, sizeof(0x15 * sizeof(uint8_t)));
    memcpy(&reginfo->gr0, s->gr, sizeof(0x9 * sizeof(uint8_t)));
    memcpy(&reginfo->cr0, s->cr, sizeof(0x19 * sizeof(uint8_t)));

    return reginfo;
}

static VgaSeqInfo* vga_qmp_query_seq(VGACommonState* s) {
    VgaSeqInfo* seqinfo = g_malloc(sizeof(VgaSeqInfo));

    return seqinfo;
}

static VgaInfo* vga_qmp_query(struct VgaInstance* vi) {
    VgaInfo* info = g_malloc(sizeof(VgaInfo));
    VGACommonState *s = (VGACommonState *)vi->vga_state;

    info->crt = vga_qmp_query_crt(s);
    info->graphics = vga_qmp_query_gfx(s);
    info->registerdump = vga_qmp_dump_regs(s);
    info->sequencer = vga_qmp_query_seq(s);

    return info;
}

VgaInfoList* qmp_query_vga(Error **errp) {
    VgaInfoList *head = NULL, **tail = &head;
    struct VgaInstance *vga_instance;

    QLIST_FOREACH(vga_instance, &vga_instances, next) {
        QAPI_LIST_APPEND(tail, vga_qmp_query(vga_instance));
    }

    return head;
}

static void hmp_printregs(Monitor* mon, uint8_t* regs, size_t len) {
    for (size_t i = 0; i < len; i++)
    {
        monitor_printf(mon, "0x%02x  ", regs[i]);
        if (!((i + 1) % 8)) {
            monitor_printf(mon, "\n   ");
        }
    }
}

void hmp_info_vga(Monitor *mon, const QDict *qdict)
{
    Error* err;

    VgaInfoList *info_list, *info;

    info_list = qmp_query_vga(&err);

    int k = 0;

    for (info = info_list; info; info = info->next) {
        VgaInfo* instance = info->value;

        monitor_printf(mon, "VGA #%i (Mode: %s)\n", k, instance->graphics->mode);

        VgaCrtInfo* crt = instance->crt;
        monitor_printf(mon, " CRT:\n");
        monitor_printf(mon, "  Res: %dx%d\n", crt->width, crt->height);
        monitor_printf(mon, "  Freq: %d Hz\n", crt->freq);
        monitor_printf(mon, "  Dots: %ld\n", crt->ticksperchar);
        monitor_printf(mon, "  Total Chars: %ld\n", crt->totalchars);
        monitor_printf(mon, "  H_scan: (s:%d, e:%d, t:%d)\n", crt->hstart, crt->hend, crt->htotal);
        monitor_printf(mon, "  V_scan: (s:%d, e:%d, t:%d)\n", crt->vstart, crt->vend, crt->vend - crt->vstart);

        VgaRegisterInfo* r = instance->registerdump;
        monitor_printf(mon, " Regsiters:\n");
        
        uint8_t *regs = (uint8_t*)&r->sr0;
        monitor_printf(mon, "  SR:\n   ");
        hmp_printregs(mon, regs, 0x5);
        
        regs = (uint8_t*)&r->gr0;
        monitor_printf(mon, "\n  GR:\n   ");
        hmp_printregs(mon, regs, 0x9);

        regs = (uint8_t*)&r->ar0;
        monitor_printf(mon, "\n  AR:\n   ");
        hmp_printregs(mon, regs, 0x15);

        regs = (uint8_t*)&r->cr0;
        monitor_printf(mon, "\n  CR:\n   ");
        hmp_printregs(mon, regs, 0x19);

        monitor_printf(mon, "\n\n");

        k++;
    }

    qapi_free_VgaInfoList(info_list);
}