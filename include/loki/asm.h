/*! \file asm.h
 *! \brief Macros useful for loki assembly code and C code too. */

#ifndef LOKI_ASM_H_
#define LOKI_ASM_H_

/* MIPS style register names, useful for porting assembly code. */
#define zero r0  /* wired zero */
#define pc   r1  /* address of current packet */
#define c2   r2  /* channels 2 to 7 */
#define c3   r3
#define c4   r4
#define c5   r5
#define c6   r6
#define c7   r7
#define sp   r8  /* stack pointer */
#define fp   r9  /* frame pointer */
#define ra   r10 /* return address */
#define v0   r11 /* return value */
#define v1   r12
#define a0   r13 /* argument 0 to 4 */
#define a1   r14
#define a2   r15
#define a3   r16
#define a4   r17
#define t0   r18 /* caller saved 0 to 4 */
#define t1   r19
#define t2   r20
#define t3   r21
#define t4   r22
#define at   r23 /* assembler temp */
#define s0   r24 /* callee saved 0 to 7 */
#define s1   r25
#define s2   r26
#define s3   r27
#define s4   r28
#define s5   r29
#define s6   r30
#define s7   r31

#endif
