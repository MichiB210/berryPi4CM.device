/*
 * berrypi4cm.device - USB host controller driver for PiStorm/Emu68 (CM4 / Pi 4, DWC2), based on berrypi3ap 105.1
 * Copyright (C) 2026 MichiB210
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Hardware layer: register access, firmware power-up, core init, root port,
 * frame counter, DMA mapping.
 *
 * Init sequence and values come from USB57..USB102 (found on real hardware)
 * and were cross-checked against Linux dwc2 core.c / params.c.
 */
#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include "dev.h"
#include "regs.h"

#ifndef DMA_ReadFromRAM
#define DMA_ReadFromRAM (1L << 3)
#endif

static inline ULONG bswap32(ULONG v)
{
    return (v >> 24) | ((v >> 8) & 0xFF00UL) | ((v << 8) & 0xFF0000UL) | (v << 24);
}

/* The DWC2 is little-endian; the 68k sees every register byte-swapped. */
ULONG hw_rd(ULONG off)
{
    return bswap32(*(volatile ULONG *)(USB_BASE + off));
}

void hw_wr(ULONG off, ULONG val)
{
    *(volatile ULONG *)(USB_BASE + off) = bswap32(val);
}

static ULONG mbox_rd(ULONG off)
{
    return bswap32(*(volatile ULONG *)(MBOX_BASE + off));
}

static void mbox_wr(ULONG off, ULONG val)
{
    *(volatile ULONG *)(MBOX_BASE + off) = bswap32(val);
}

static BOOL wait_set(ULONG off, ULONG mask)
{
    ULONG n;
    for (n = 0; n < 0x800000UL; n++)
        if (hw_rd(off) & mask)
            return TRUE;
    return FALSE;
}

static BOOL wait_clear(ULONG off, ULONG mask)
{
    ULONG n;
    for (n = 0; n < 0x800000UL; n++)
        if (!(hw_rd(off) & mask))
            return TRUE;
    return FALSE;
}

/* ---------------------------------------------------------------- DMA */

APTR hw_dma_pre(APTR addr, ULONG len, BOOL to_device, ULONG *bus)
{
    ULONG l = len ? len : 4;
    APTR phys = CachePreDMA(addr, &l, to_device ? DMA_ReadFromRAM : 0);
    if (len && l < len) {
        CachePostDMA(addr, &l, to_device ? DMA_ReadFromRAM : 0);
        return NULL;            /* not physically contiguous */
    }
    /* berrypi4cm 1.0: on BCM2711 the DWC2 only reaches the first GB
     * (bus alias 0xC0000000 | phys).  A buffer above it would be cut
     * off by the mask below and the controller would write somewhere
     * else - refuse it instead.  All DMA goes through our own buffer
     * (allocated once at start), so this only matters on 2 GB+ boards. */
    if ((ULONG)phys >= 0x40000000UL || (ULONG)phys + l > 0x40000000UL) {
        CachePostDMA(addr, &l, to_device ? DMA_ReadFromRAM : 0);
        drv.st.dma_high++;
        return NULL;
    }
    /* VideoCore bus alias for uncached SDRAM (USB57) */
    *bus = ((ULONG)phys & 0x3FFFFFFFUL) | 0xC0000000UL;
    return phys ? phys : (APTR)1;
}

void hw_dma_post(APTR addr, ULONG len, BOOL to_device)
{
    ULONG l = len ? len : 4;
    CachePostDMA(addr, &l, to_device ? DMA_ReadFromRAM : 0);
}

/* ------------------------------------------------------------ mailbox */

/* Firmware property call: send the 'len'-byte buffer drv.prop on the
 * property channel and wait for the answer (as in USB57). */
static LONG mbox_prop(ULONG len)
{
    ULONG *p = drv.prop;
    ULONG l = len, n, v, want;
    APTR phys;

    phys = CachePreDMA(p, &l, 0);
    want = ((ULONG)phys & ~0xFUL) | MBOX_CH_PROP;

    for (n = 0; n < 0x800000UL; n++)
        if (!(mbox_rd(MBOX_STATUS) & MBOX_FULL))
            break;
    if (n == 0x800000UL)
        goto fail;
    mbox_wr(MBOX_WRITE, want);

    for (n = 0; n < 0x800000UL; n++) {
        if (mbox_rd(MBOX_STATUS) & MBOX_EMPTY)
            continue;
        v = mbox_rd(MBOX_READ);
        if ((v & 0xF) == MBOX_CH_PROP && (v & ~0xFUL) == (want & ~0xFUL)) {
            l = len;
            CachePostDMA(p, &l, 0);
            return 0;
        }
    }
fail:
    l = len;
    CachePostDMA(p, &l, 0);
    return -1;
}

/* SET_POWER(USB HCD, on + wait), as in USB57. */
static LONG mbox_power_usb(void)
{
    ULONG *p = drv.prop;

    p[0] = bswap32(32);
    p[1] = 0;
    p[2] = bswap32(TAG_SET_POWER);
    p[3] = bswap32(8);
    p[4] = bswap32(8);
    p[5] = bswap32(POWER_DEV_USB);
    p[6] = bswap32(3);          /* on, wait for stable */
    p[7] = 0;
    return mbox_prop(32);
}

/* berrypi4cm 1.0: GET_BOARD_REVISION.  Returns 0 if unknown. */
static ULONG mbox_board_rev(void)
{
    ULONG *p = drv.prop;

    p[0] = bswap32(28);
    p[1] = 0;
    p[2] = bswap32(TAG_GET_BOARD_REV);
    p[3] = bswap32(4);
    p[4] = 0;
    p[5] = 0;
    p[6] = 0;
    if (mbox_prop(28))
        return 0;
    if (bswap32(p[1]) != 0x80000000UL)      /* firmware: request ok */
        return 0;
    return bswap32(p[5]);
}

/* --------------------------------------------------------- core init */

LONG hw_init(void)
{
    ULONG v, id, ch;

    /* berrypi4cm 1.0: this driver is for BCM2711 (CM4 / Pi 4).  New-style
     * revision codes (bit 23) carry the SoC in bits 12-15: 3 = BCM2711.
     * Unknown revision: carry on as BCM2711 (do not lock anyone out). */
    {
        ULONG rev = mbox_board_rev();
        drv.st.board_rev = rev;
        if (rev && (rev & 0x00800000UL) && ((rev >> 12) & 0xF) != 3)
            return -7;          /* Pi 3 or older: use berrypi3ap.device */
        irq_no_armctrl = TRUE;
    }

    irq_pi_off();       /* 104.3: never start with a left-over USB interrupt */

    /* berrypi4cm 1.0: our DMA buffer must be in the first GB */
    {
        ULONG l = drv.dma_alloc_size;
        APTR phys = CachePreDMA(drv.dma_alloc, &l, 0);
        CachePostDMA(drv.dma_alloc, &l, 0);
        if ((ULONG)phys + drv.dma_alloc_size > 0x40000000UL)
            return -8;
    }

    if (mbox_power_usb())
        return -1;

    id = hw_rd(GSNPSID) & 0xFFFFF000UL;
    if (id != 0x4F542000UL && id != 0x4F543000UL)
        return -2;

    hw_wr(GINTMSK, 0);
    hw_wr(GAHBCFG, hw_rd(GAHBCFG) & ~1UL);         /* global int off */
    hw_wr(GUSBCFG, hw_rd(GUSBCFG) & 0xFFAFFFFFUL);

    if (!wait_set(GRSTCTL, GRSTCTL_AHBIDLE))
        return -3;
    hw_wr(GRSTCTL, hw_rd(GRSTCTL) | GRSTCTL_CSFTRST);
    if (!wait_clear(GRSTCTL, GRSTCTL_CSFTRST))
        return -4;
    hw_delay_ms(100);

    /* UTMI+ 8 bit, no ULPI, no SRP/HNP (USB57) */
    v = hw_rd(GUSBCFG);
    v &= 0xFFF7FFF7UL;
    v &= 0xFFFDFCEFUL;
    hw_wr(GUSBCFG, v);

    /* force host mode (Linux dwc2_force_mode, USB102) */
    v = hw_rd(GUSBCFG);
    v &= ~GUSBCFG_FORCEDEV;
    v |= GUSBCFG_FORCEHOST;
    hw_wr(GUSBCFG, v);
    hw_delay_ms(100);

    /* AHB: BCM value 0x10 + buffer DMA (Linux bcm params, USB57) */
    v = hw_rd(GAHBCFG) & ~0x7UL;
    v |= GAHBCFG_BCM_AXI | GAHBCFG_DMA_EN;
    hw_wr(GAHBCFG, v);

    hw_wr(PCGCTL, 0);
    hw_wr(HCFG, hw_rd(HCFG) & ~3UL);               /* 30/60 MHz PHY clock */

    /* FIFO layout proven since USB57 (fits the 4080-word FIFO RAM) */
    hw_wr(GRXFSIZ, 0x00000400UL);
    hw_wr(GNPTXFSIZ, 0x04000400UL);
    hw_wr(HPTXFSIZ, 0x04000800UL);

    hw_wr(GRSTCTL, hw_rd(GRSTCTL) | GRSTCTL_TXFFLSH | GRSTCTL_TXFNUM_ALL);
    if (!wait_clear(GRSTCTL, GRSTCTL_TXFFLSH))
        return -5;
    hw_wr(GRSTCTL, hw_rd(GRSTCTL) | GRSTCTL_RXFFLSH);
    if (!wait_clear(GRSTCTL, GRSTCTL_RXFFLSH))
        return -6;

    for (ch = 0; ch < 8; ch++) {
        hw_wr(HCINTMSK(ch), 0);
        hw_wr(HCINT(ch), 0xFFFFFFFFUL);
        hw_wr(HCSPLT(ch), 0);
    }

    hw_port_power();
    return 0;
}

/* ----------------------------------------------------------- root port */

ULONG hw_hprt(void)
{
    return hw_rd(HPRT0);
}

/* Read-modify-write without accidentally clearing W1C bits or disabling
 * the port; set_bits may include W1C bits on purpose. */
void hw_hprt_write(ULONG set_bits, ULONG clear_bits)
{
    ULONG v = hw_rd(HPRT0) & ~HPRT_W1C_MASK;
    v &= ~clear_bits;
    v |= set_bits;
    hw_wr(HPRT0, v);
}

void hw_port_power(void)
{
    hw_hprt_write(HPRT_PWR, 0);
    hw_delay_ms(40);
}

/* USB101 prepare_root_port timings (proven on hardware). */
LONG hw_port_reset(void)
{
    ULONG v;

    hw_hprt_write(HPRT_PWR, 0);
    hw_delay_ms(500);
    v = hw_hprt();
    if (!(v & HPRT_PWR) || !(v & HPRT_CONN))
        return -1;
    hw_delay_ms(100);

    hw_hprt_write(HPRT_PWR | HPRT_RST, 0);
    hw_delay_ms(100);                       /* >= 50 ms (USB 2.0, Linux) */
    hw_hprt_write(HPRT_PWR, HPRT_RST);
    hw_delay_ms(40);                        /* >= 10 ms recovery */

    v = hw_hprt();
    if (!(v & HPRT_CONN) || !(v & HPRT_ENA) || (v & HPRT_RST))
        return -2;

    drv.port_speed = (UBYTE)((v & HPRT_SPD_MASK) >> 17);
    return 0;
}

/* ------------------------------------------------------- frame counter */

/* HFNUM counts microframes on a high-speed port, frames otherwise. */
UWORD hw_frnum(void)
{
    return (UWORD)(hw_rd(HFNUM) & 0x3FFF);
}

ULONG hw_ticks_per_ms(void)
{
    return drv.port_speed == 0 ? 8 : 1;
}

UWORD hw_frame_diff(UWORD later, UWORD earlier)
{
    return (UWORD)((later - earlier) & 0x3FFF);
}

/* Free-running millisecond clock built from the 14-bit frame counter.
 * Must be called at least once per counter wrap (~2 s at high speed);
 * the unit task calls it on every scheduling round. */
ULONG hw_ms_now(void)
{
    static UWORD last;
    static ULONG ticks_acc, ms;
    UWORD now = hw_frnum();
    ULONG tpm = hw_ticks_per_ms();

    ticks_acc += hw_frame_diff(now, last);
    last = now;
    ms += ticks_acc / tpm;
    ticks_acc %= tpm;
    return ms;
}

void hw_wait_next_tick(void)
{
    UWORD f = hw_frnum();
    ULONG n;
    for (n = 0; n < 0x20000UL; n++)
        if (hw_frnum() != f)
            return;
}
