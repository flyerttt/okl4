/*
 * Copyright (c) 2006, National ICT Australia (NICTA)
 */
/*
 * Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
 * All rights reserved.
 *
 * 1. Redistribution and use of OKL4 (Software) in source and binary
 * forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 *     (a) Redistributions of source code must retain this clause 1
 *         (including paragraphs (a), (b) and (c)), clause 2 and clause 3
 *         (Licence Terms) and the above copyright notice.
 *
 *     (b) Redistributions in binary form must reproduce the above
 *         copyright notice and the Licence Terms in the documentation and/or
 *         other materials provided with the distribution.
 *
 *     (c) Redistributions in any form must be accompanied by information on
 *         how to obtain complete source code for:
 *        (i) the Software; and
 *        (ii) all accompanying software that uses (or is intended to
 *        use) the Software whether directly or indirectly.  Such source
 *        code must:
 *        (iii) either be included in the distribution or be available
 *        for no more than the cost of distribution plus a nominal fee;
 *        and
 *        (iv) be licensed by each relevant holder of copyright under
 *        either the Licence Terms (with an appropriate copyright notice)
 *        or the terms of a licence which is approved by the Open Source
 *        Initative.  For an executable file, "complete source code"
 *        means the source code for all modules it contains and includes
 *        associated build and other files reasonably required to produce
 *        the executable.
 *
 * 2. THIS SOFTWARE IS PROVIDED ``AS IS'' AND, TO THE EXTENT PERMITTED BY
 * LAW, ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, OR NON-INFRINGEMENT, ARE DISCLAIMED.  WHERE ANY WARRANTY IS
 * IMPLIED AND IS PREVENTED BY LAW FROM BEING DISCLAIMED THEN TO THE
 * EXTENT PERMISSIBLE BY LAW: (A) THE WARRANTY IS READ DOWN IN FAVOUR OF
 * THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
 * PARTICIPANT) AND (B) ANY LIMITATIONS PERMITTED BY LAW (INCLUDING AS TO
 * THE EXTENT OF THE WARRANTY AND THE REMEDIES AVAILABLE IN THE EVENT OF
 * BREACH) ARE DEEMED PART OF THIS LICENCE IN A FORM MOST FAVOURABLE TO
 * THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
 * PARTICIPANT). IN THE LICENCE TERMS, "PARTICIPANT" INCLUDES EVERY
 * PERSON WHO HAS CONTRIBUTED TO THE SOFTWARE OR WHO HAS BEEN INVOLVED IN
 * THE DISTRIBUTION OR DISSEMINATION OF THE SOFTWARE.
 *
 * 3. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ANY OTHER PARTICIPANT BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Description:   Periodic timer handling
 */

#include <soc/soc.h>
#include <soc/interface.h>
#include <interrupt.h>
#include <reg.h>
#include <soc.h>

addr_t versatile_timer0_vbase;
addr_t versatile_sctl_vbase;

//#define TRACE_TIMER SOC_TRACEF
#define TRACE_TIMER(x...)

#if defined(CONFIG_KDEBUG_TIMER)
word_t soc_get_timer_tick_length(void)
{
    return TIMER_TICK_LENGTH;
}
#endif

void handle_timer_interrupt(bool wakeup, continuation_t continuation)
{
    volatile Timer_t    *timer0 = (Timer_t *)VERSATILE_TIMER0_VBASE;
    TRACE_TIMER("irq:%d context:%t\n", irq, context);

    /* clear interrupt by writing any value to clear register
     *
     * This used to be after the handle timer interrupt call, but this produced
     * broken behaviour, in that a high priority thread could miss it's timeslice
     * by the interrupt being reserviced before it ran.
     */
    timer0->clear = ~(0UL);

    kernel_scheduler_handle_timer_interrupt(wakeup,
                                            TIMER_TICK_LENGTH,
                                            continuation);
}

void init_clocks(void)
{
    volatile Timer_t    *timer0 = (Timer_t *)VERSATILE_TIMER0_VBASE;
    volatile Timer_t    *timer1 = (Timer_t *)VERSATILE_TIMER1_VBASE;
    volatile SysCtl_t   *sysctl = (SysCtl_t *)VERSATILE_SCTL_VBASE;

    /*
     * set clock frequency:
     *      VERSATILE_REFCLK is 32KHz
     *      VERSATILE_TIMCLK is 1MHz
     */
    sysctl->ctrl = VERSATILE_CLOCK_MODE_NORMAL |
                    (1UL << VERSATILE_BIT_TIMER_EN0_SEL) |
                    (1UL << VERSATILE_BIT_TIMER_EN1_SEL) |
                    (1UL << VERSATILE_BIT_TIMER_EN2_SEL) |
                    (1UL << VERSATILE_BIT_TIMER_EN3_SEL);

    /* Enable 5ms L4 system tick */
    timer0->load = VERSATILE_TIMER_RELOAD;
    TRACE_TIMER("Timer reload val:0x%08x\n", VERSATILE_TIMER_RELOAD);

    /* enable irq.*/
    soc_unmask_irq(VERSATILE_TIMER0_IRQ);

    /* start timer - enable + clkdiv + periodic + int enable */
    timer0->ctrl = VERSATILE_TIMER_CTRL | VERSATILE_TIMER_MODE | VERSATILE_TIMER_IE;
    TRACE_TIMER("Timer ctrl val:0x%08x\n", timer0->ctrl);

    /* Enable 1MHz free-running timer */
    timer1->ctrl = VERSATILE_TIMER_CTRL | VERSATILE_TIMER_32BIT;
    timer1->clear = 0;

}
