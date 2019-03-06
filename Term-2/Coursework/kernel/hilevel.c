/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

/* We assume there will be 2 user processes, stemming from the 2 user programs,
 * and so can
 *
 * - allocate a fixed-size process table (of PCBs), and then maintain an index
 *   into it to keep track of the currently executing process, and
 * - employ a fixed-case of round-robin scheduling: no more processes can be
 *   created, and neither can be terminated, so assume both are always ready
 *   to execute.
 */

pcb_t pcb[ 3 ]; pcb_t* current = NULL;
int amountPrograms = 3;

void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {
  char prev_pid = '?', next_pid = '?';

  if( NULL != prev ) {
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) ); // preserve execution context of P_{prev}
    prev_pid = '0' + prev->pid;
  }
  if( NULL != next ) {
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) ); // restore  execution context of P_{next}
    next_pid = '0' + next->pid;
  }

    PL011_putc( UART0, '[',      true );
    PL011_putc( UART0, prev_pid, true );
    PL011_putc( UART0, '-',      true );
    PL011_putc( UART0, '>',      true );
    PL011_putc( UART0, next_pid, true );
    PL011_putc( UART0, ']',      true );

    current = next;                             // update   executing index   to P_{next}

  return;
}

void priority_schedule( ctx_t* ctx ) {
  int highest_priority = 0;
  int second_priority = 0;

  for (size_t i = 0; i < amountPrograms; i++) {     //Find out which has the highest priority
    if (pcb[i].working_priority > pcb[highest_priority].working_priority) {
      highest_priority = i;
    }
  }


  for (size_t i = 0; i < amountPrograms; i++) {     //Increase the priority of all the other programs but the one that is about to be executed
    if (i != highest_priority) {
      pcb[i].working_priority++;
    }
  }

  for (size_t i = 0; i < amountPrograms; i++) {     //Find out what the next program to be executed is going to be
    if (pcb[i].working_priority > pcb[second_priority].working_priority) {
      second_priority = i;
    }
  }
  
  dispatch(ctx, &pcb[highest_priority], &pcb[second_priority]);   // context switch P_i -> P_i+1
  pcb[highest_priority].status = STATUS_READY;  // update   execution status  of P_i
  pcb[second_priority].status = STATUS_EXECUTING;  // update   execution status  of P_i+1

  return;
}

void round_robin_schedule( ctx_t* ctx ) {

  for (size_t i = 0; i < amountPrograms; i++) {
    if     ( current->pid == pcb[ i ].pid ) {
      int nextProgram = (i+1)%amountPrograms;
      dispatch( ctx, &pcb[ i ], &pcb[ nextProgram ] );      // context switch P_i -> P_i+1

      pcb[ i ].status = STATUS_READY;             // update   execution status  of P_i
      pcb[ nextProgram ].status = STATUS_EXECUTING;         // update   execution status  of P_i+1
      break;
    }
  }

  return;
}

extern void     main_P3();
extern uint32_t tos_P3;
extern void     main_P4();
extern uint32_t tos_P4;
extern void     main_P5();
extern uint32_t tos_P5;

void hilevel_handler_rst( ctx_t* ctx              ) {
  /* Initialise PCBs, representing user processes stemming from execution
   * of 3 user programs.  Note in each case that
   *
   * - the CPSR value of 0x50 means the processor is switched into USR mode,
   *   with IRQ interrupts enabled, and
   * - the PC and SP values matche the entry point and top of stack.
   */

  memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );     // initialise 0-th PCB = P_3
  pcb[ 0 ].pid      = 3;
  pcb[ 0 ].status   = STATUS_CREATED;
  pcb[ 0 ].ctx.cpsr = 0x50;
  pcb[ 0 ].ctx.pc   = ( uint32_t )( &main_P3 );
  pcb[ 0 ].ctx.sp   = ( uint32_t )( &tos_P3  );
  pcb[ 0 ].static_priority = 5;
  pcb[ 0 ].working_priority = 5;

  memset( &pcb[ 1 ], 0, sizeof( pcb_t ) );     // initialise 2-nd PCB = P_4
  pcb[ 1 ].pid      = 4;
  pcb[ 1 ].status   = STATUS_CREATED;
  pcb[ 1 ].ctx.cpsr = 0x50;
  pcb[ 1 ].ctx.pc   = ( uint32_t )( &main_P4 );
  pcb[ 1 ].ctx.sp   = ( uint32_t )( &tos_P4  );
  pcb[ 1 ].static_priority = 3;
  pcb[ 1 ].working_priority = 3;

  memset( &pcb[ 2 ], 0, sizeof( pcb_t ) );     // initialise 3-rd PCB = P_5
  pcb[ 2 ].pid      = 5;
  pcb[ 2 ].status   = STATUS_CREATED;
  pcb[ 2 ].ctx.cpsr = 0x50;
  pcb[ 2 ].ctx.pc   = ( uint32_t )( &main_P5 );
  pcb[ 2 ].ctx.sp   = ( uint32_t )( &tos_P5  );
  pcb[ 2 ].static_priority = 1;
  pcb[ 2 ].working_priority = 1;

  TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

  GICC0->PMR          = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
  GICC0->CTLR         = 0x00000001; // enable GIC interface
  GICD0->CTLR         = 0x00000001; // enable GIC distributor

  /* Once the PCBs are initialised, we arbitrarily select the one in the 0-th
   * PCB to be executed: there is no need to preserve the execution context,
   * since it is is invalid on reset (i.e., no process will previously have
   * been executing).
   */#
  dispatch( ctx, NULL, &pcb[ 0 ] );
  int_enable_irq();


  return;
}

void hilevel_handler_irq( ctx_t* ctx) {
  /* Based on the identifier (i.e., the immediate operand) extracted from the
   * svc instruction,
   *
   * - read  the arguments from preserved usr mode registers,
   * - perform whatever is appropriate for this system call, then
   * - write any return value back to preserved usr mode registers.
   */


   // Step 2: read  the interrupt identifier so we know the source.

   uint32_t id = GICC0->IAR;

   // Step 4: handle the interrupt, then clear (or reset) the source.

   if( id == GIC_SOURCE_TIMER0 ) {
     priority_schedule( ctx ); TIMER0->Timer1IntClr = 0x01;
   }

   // Step 5: write the interrupt identifier to signal we're done.

   GICC0->EOIR = id;

   return;
}

void hilevel_handler_svc(ctx_t* ctx){

  return;
}
