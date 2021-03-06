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
 * Description:   Assembler macros for GNU tools
 */
#ifndef __ARCH__ARM__ASM_GNU_H__
#define __ARCH__ARM__ASM_GNU_H__

//#include <cpu/syscon.h>

/* XXX: inline asm hacks */
#define _(x)                    "%["#x"]"

/* calls / branches */
#if defined(CONFIG_ARM_V5) || defined(CONFIG_ARM_V6) || (CONFIG_ARM_VER >= 6)

#if defined(ASSEMBLY)
    .macro jump reg
    bx      \reg
    .endm
    .macro call reg
    blx     \reg
    .endm
#else
#define jump                    bx
#define call                    blx
#endif

#else

#if defined(ASSEMBLY)
    .macro jump reg
    mov     pc, \reg
    .endm
    .macro call reg
    mov     lr, pc
    mov     pc, \reg
    .endm
#else
#define jump                    mov     pc,
#define call                    mov     lr, pc; mov     pc,
#endif

#endif

#if defined(ASSEMBLY)
/* ARM style directives */
#define ALIGN                   .balign
#define CODE16                  .force_thumb
#define DCDU                    .word
#define END
#define EXPORT                  .global
#define IMPORT                  .extern
#define LABEL(name)             name:
#define LTORG                   .ltorg
#define MACRO                   .macro
#define MEND                    .endm

#define BEGIN_PROC(name)                        \
    .global name;                               \
    .type   name,function;                      \
    .align;                                     \
name:

#define END_PROC(name)                          \
3999:                                           \
    .size   name,3999b - name;

/* Functions to generate symbols in the output file
 * with correct relocated address for debugging
 */
#define TRAPS_BEGIN_MARKER                      \
    .section .data.traps;                       \
    .balign 4096;                               \
    __vector_vaddr:

#define VECTOR_WORD(name)                       \
    .equ    vector_##name, (name - __vector_vaddr + 0xffff0000);        \
    .global vector_##name;                      \
    .type   vector_##name,object;               \
    .size   vector_##name,4;                    \
name:

#define BEGIN_PROC_TRAPS(name)                  \
    .global name;                               \
    .type   name,function;                      \
    .equ    vector_##name, (name - __vector_vaddr + 0xffff0000);        \
    .global vector_##name;                      \
    .type   vector_##name,function;             \
    .align;                                     \
name:

#define END_PROC_TRAPS(name)                    \
3999:                                           \
    .size   name,3999b - name;                  \
    .equ    .fend_vector_##name, (3999b - __vector_vaddr + 0xffff0000);  \
    .size   vector_##name,.fend_vector_##name - vector_##name;

#endif

#define CHECK_ARG(a, b)                         \
        "   .ifnc " a ", " b "  \n"             \
        "   .err                \n"             \
        "   .endif              \n"             \

#if CONFIG_ARM_VER >= 6
/* ARMv6 Convenience Macros */
#define r13_svc     #0x13
#define svc_mode    #0x13

#define r13_abt     #0x17
#define abt_mode    #0x17
#endif

#endif /* __ARCH__ARM__ASM_GNU_H__ */
