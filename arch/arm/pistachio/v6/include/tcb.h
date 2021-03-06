/*
 * Copyright (c) 2003-2006, National ICT Australia (NICTA)
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
 * Description:   ARMv6 specific TCB handling functions
 */
#ifndef __ARM__V6__TCB_H__
#define __ARM__V6__TCB_H__

#ifndef __TCB_H__
#error not for stand-alone inclusion
#endif

#if defined (__GNUC__)

#define asm_initial_switch_to(new_sp, new_pc)                               \
    __asm__ __volatile__ (                                                  \
        "    mov     sp,     %0              \n" /* load new stack ptr */   \
        "    mov     pc,     %1              \n" /* load new PC */          \
        :: "r" (new_sp), "r" (new_pc)                                       \
        : "memory")

#elif defined (__RVCT_GNU__)

#define asm_initial_switch_to(new_sp, new_pc)   \
    jump_and_set_sp((word_t)new_pc, new_sp)

#elif defined(_lint)
#define asm_initial_switch_to(new_sp, new_pc) jump_and_set_sp((word_t)new_pc, new_sp)
#else
#error unknown compiler
#endif


/**********************************************************************
 *
 *                      thread switch routines
 *
 **********************************************************************/

/**
 * switch to initial thread
 * @param tcb TCB of initial thread
 *
 * Initializes context of initial thread and switches to it.  The
 * context (e.g., instruction pointer) has been generated by inserting
 * a notify procedure context on the stack.  We simply restore this
 * context.
 */

INLINE void NORETURN initial_switch_to (tcb_t * tcb)
{
    tcb_t ** stack_top;
    space_t *space = get_kernel_space();
    hw_asid_t new_asid = space->get_asid()->get(space);

    word_t new_pt = space->pgbase;

    arm_cache::cache_flush();

#if !defined(CONFIG_ARM_THREAD_REGISTERS)
    USER_UTCB_REF = tcb->get_utcb_location();
#else
    /* UTCB: Update User-RO Thread Register with UTCB */
    write_cp15_register(C15_pid, c0, 0x3, tcb->get_utcb_location());
#endif

    /* Flush BTB/BTAC */
    write_cp15_register(C15_cache_con, c5, 0x6, 0x0);
    /* drain write buffer */
    write_cp15_register(C15_cache_con, c10, 0x4, 0x0);
    /* install new PT */
    write_cp15_register(C15_ttbase, C15_CRm_default, 0, new_pt);
    /* flush TLB */
    write_cp15_register(C15_tlb, c7, C15_OP2_default, 0x0);
    /* Set new ASID (procID) */
    write_cp15_register(C15_pid, c0, 0x1, new_asid);

    /* keep stack aligned for RVCT */
    stack_top = ((tcb_t **)(&__stack + 1))-2;

    /* switch */
    asm_initial_switch_to((addr_t)stack_top, tcb->cont);

    ASSERT(ALWAYS, !"We shouldn't get here!");
    while(true) {}
}

/**********************************************************************
 *
 *            access functions for ex-regs'able registers
 *
 **********************************************************************/

/**
 * read the user-level instruction pointer
 * @return      the user-level stack pointer
 */
INLINE addr_t tcb_t::get_user_ip()
{
    arm_irq_context_t * context = &arch.context;

    return (addr_t) ((context)->pc & ~1UL);
}


/**
 * set the user-level instruction pointer
 * @param ip    new user-level instruction pointer
 */
INLINE void tcb_t::set_user_ip(addr_t ip)
{
    word_t new_pc = (word_t)ip;
    arm_irq_context_t *context = &arch.context;

#ifdef CONFIG_ARM_THUMB_SUPPORT
    /* CPU has thumb support, fix cpsr if needed */
    if (new_pc & 1) {
        context->cpsr |= CPSR_THUMB_BIT;
    } else {
        context->cpsr &= ~CPSR_THUMB_BIT;
    }
#endif

    /* Is thread on a syscall? */
    if (context->pc & 1)
        new_pc |= 1;
    else
        new_pc &= ~1UL;

    context->pc = new_pc;
}

#endif /* !__ARM__V6__TCB_H__ */
