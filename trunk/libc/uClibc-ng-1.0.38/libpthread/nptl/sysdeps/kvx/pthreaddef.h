/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2019 Kalray Inc.
 */

/* Default stack size.  */
#define ARCH_STACK_DEFAULT_SIZE	(2 * 1024 * 1024)

/* Required stack pointer alignment at beginning.  */
#define STACK_ALIGN 32

/* Minimal stack size after allocating thread descriptor and guard size.  */
#define MINIMAL_REST_STACK 2048

/* Alignment requirement for TCB.  */
#define TCB_ALIGNMENT 32

/* Location of current stack frame.  */
#define CURRENT_STACK_FRAME	__builtin_frame_address (0)

/* XXX Until we have a better place keep the definitions here.  */
#define __exit_thread_inline(val) \
  INLINE_SYSCALL (exit, 1, (val))
