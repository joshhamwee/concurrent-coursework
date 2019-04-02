/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"
int numberprocesses;
pcb_t pcb[ 32 ]; pcb_t* current = NULL;
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

int current_pid(){
  for (size_t i = 0; i < numberprocesses; i++) {
    if(pcb[i].pid == current->pid){
      return i;
    }
  }
}

void age_all(){
  for (size_t i = 0; i < numberprocesses; i++) {
    if(pcb[i].status != STATUS_TERMINATED){
          pcb[i].age++;
    }
  }
}

void priority_schedule( ctx_t* ctx){
  age_all();
  int highest_priority_pcb = 0;
  for (size_t i = 0; i < numberprocesses; i++) {
    if (pcb[i].priority + pcb[i].age > pcb[highest_priority_pcb].priority + pcb[highest_priority_pcb].priority && (pcb[i].status != STATUS_TERMINATED)) {
      highest_priority_pcb = i;
    }
  }

  if(current_pid() == highest_priority_pcb){
    return;
  }
  else{
    dispatch(ctx, &pcb[current_pid()],&pcb[highest_priority_pcb]);
  }

  pcb[current_pid()].status = STATUS_READY;
  pcb[highest_priority_pcb].status = STATUS_EXECUTING;
  pcb[highest_priority_pcb].age = 0;
  current = &pcb[highest_priority_pcb];
  return;
}

void HL_write(ctx_t* ctx){
  int   fd = ( int   )( ctx->gpr[ 0 ] );
  char*  x = ( char* )( ctx->gpr[ 1 ] );
  int    n = ( int   )( ctx->gpr[ 2 ] );

  for( int i = 0; i < n; i++ ) {
    PL011_putc( UART0, *x++, true );
  }

  ctx->gpr[ 0 ] = n;
}

void HL_read(ctx_t* ctx){
  int   fd = ( int   )( ctx->gpr[ 0 ] );
  char*  x = ( char* )( ctx->gpr[ 1 ] );
  int    n = ( int   )( ctx->gpr[ 2 ] );

  for( int i = 0; i < n; i++ ) {
    x[i] = PL011_getc(UART0, true);
  }

  ctx->gpr[ 0 ] = n;
}

void HL_exit(ctx_t* ctx){
    pcb[current_pid()].status = STATUS_TERMINATED;
    pcb[current_pid()].priority = 0;
    pcb[current_pid()].age = 0;
    priority_schedule(ctx);
}
extern void main_P3();
extern uint32_t tos_P3();
extern void main_P4();
extern uint32_t tos_P4();
extern void main_P5();
extern uint32_t tos_P5();

void hilevel_handler_rst( ctx_t* ctx              ) {
  /* Initialise PCBs, representing user processes stemming from execution
   * of 3 user programs.  Note in each case that
   *
   * - the CPSR value of 0x50 means the processor is switched into USR mode,
   *   with IRQ interrupts enabled, and
   * - the PC and SP values matche the entry point and top of stack.
   */
   memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );     // initialise 0-th PCB = P_3
   pcb[ 0 ].pid      = 0;
   pcb[ 0 ].status   = STATUS_CREATED;
   pcb[ 0 ].ctx.cpsr = 0x50;
   pcb[ 0 ].ctx.pc   = ( uint32_t )( &main_P3 );
   pcb[ 0 ].ctx.sp   = ( uint32_t )( &tos_P3  );
   pcb[ 0 ].priority = 2;
   pcb[ 0 ].age = 0;

   memset( &pcb[ 1 ], 0, sizeof( pcb_t ) );     // initialise 0-th PCB = P_3
   pcb[ 1 ].pid      = 1;
   pcb[ 1 ].status   = STATUS_CREATED;
   pcb[ 1 ].ctx.cpsr = 0x50;
   pcb[ 1 ].ctx.pc   = ( uint32_t )( &main_P4 );
   pcb[ 1 ].ctx.sp   = ( uint32_t )( &tos_P4  );
   pcb[ 1 ].priority = 3;
   pcb[ 1 ].age = 0;

   memset( &pcb[ 2 ], 0, sizeof( pcb_t ) );     // initialise 0-th PCB = P_3
   pcb[ 2 ].pid      = 2;
   pcb[ 2 ].status   = STATUS_CREATED;
   pcb[ 2 ].ctx.cpsr = 0x50;
   pcb[ 2 ].ctx.pc   = ( uint32_t )( &main_P5 );
   pcb[ 2 ].ctx.sp   = ( uint32_t )( &tos_P5  );
   pcb[ 2 ].priority = 4;
   pcb[ 2 ].age = 0;

  numberprocesses = 3;
  dispatch(ctx, NULL, &pcb[0]);
  pcb[0].status = STATUS_EXECUTING;

  TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

  GICC0->PMR          = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
  GICC0->CTLR         = 0x00000001; // enable GIC interface
  GICD0->CTLR         = 0x00000001; // enable GIC distributor

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

void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {
  /* Based on the identifier (i.e., the immediate operand) extracted from the
   * svc instruction,
   *
   * - read  the arguments from preserved usr mode registers,
   * - perform whatever is appropriate for this system call, then
   * - write any return value back to preserved usr mode registers.
   */

  switch( id ) {
    case SYS_YIELD : { // 0x00 => yield()
      priority_schedule( ctx );
      break;
    }

    case SYS_WRITE : { // 0x01 => write( fd, x, n )
      HL_write(ctx);
      break;
    }

    case SYS_READ : { // 0x02 => read( fd, x, n )
      HL_read(ctx);
    }
    case SYS_FORK : { // 0x03 => fork()

      break;
    }

    case SYS_EXIT : { // 0x04 => exit(int x)
      HL_exit(ctx);
      break;
    }

    case SYS_EXEC : { // 0x05 => exec(const void* x)

      break;
    }
    default   : { // 0x?? => unknown/unsupported
      break;
    }
  }

  return;
}
