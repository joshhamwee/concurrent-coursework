/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"
int numberprocesses = 0;
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

//Return the integer pid of the currently executing program
int current_pid(){
  for (size_t i = 0; i < maximumPrograms; i++) {
    if(pcb[i].pid == current->pid){
      return i;
    }
  }
}

//Increase the age of all the processes
void age_all(){
  for (size_t i = 0; i < maximumPrograms; i++) {
    if(pcb[i].status != STATUS_TERMINATED ){
          pcb[i].age++;
    }
  }
}

//Schudler for deciding which process to run at a specific time
void priority_schedule( ctx_t* ctx){
  age_all();

  //Find the program with the highest priority
  int highest_priority_pcb = 0;
  for (size_t i = 0; i < maximumPrograms; i++) {
    if (pcb[i].priority + pcb[i].age >= pcb[highest_priority_pcb].priority + pcb[highest_priority_pcb].age && (pcb[i].status != STATUS_TERMINATED)) {
      highest_priority_pcb = i;
    }
  }

  //Return false if highest priority program is the currently executing one
  //Otherwise dispatch the highest priority one
  if(current == &pcb[highest_priority_pcb]){
    pcb[highest_priority_pcb].age = 0;
    return;
  }
  else{
    dispatch(ctx, current, &pcb[highest_priority_pcb]);
  }

  //Change status of previous and next process
  pcb[current_pid()].status = STATUS_READY;
  pcb[highest_priority_pcb].status = STATUS_EXECUTING;
  pcb[highest_priority_pcb].age = 0;
  current = &pcb[highest_priority_pcb];
  return;
}

//Find position on stack for a given pid
uint32_t giveStack(pid_t pid){
  uint32_t general = (uint32_t) (&tos_general);
  uint32_t value = general -(0x00001000 * pid);
  return value;
}

//Duplicate a parent process block into a child block with the same process
void copy_pcb_fork(pcb_t* parent , pcb_t* child , ctx_t* ctx ){
  memcpy( &child->ctx , ctx , sizeof( ctx_t ));
  child->status = STATUS_READY;
  child->age = 0;
  child->priority = 3;

  uint32_t offset = giveStack(parent->pid) - ctx->sp;
  child->ctx.sp = giveStack(child->pid) - offset;
  memcpy((void*)(child->ctx.sp), (void*)(ctx->sp),offset);
  return;
}

//Find the next empty slot within the pcb
pcb_t* next_empty_pcb(){
  int next_empty_pos = numberprocesses;
  int i = 0;
  while (i < maximumPrograms) {
    if (pcb[i].status == STATUS_TERMINATED) {
      next_empty_pos = i;
      break;
    }
    i++;
  }
  return &pcb[next_empty_pos];
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


void HL_fork(ctx_t* ctx){
  pcb_t* parent = current;
  pcb_t* child = next_empty_pcb();
  //Duplicate the parent block
  copy_pcb_fork(parent,child,ctx);

  numberprocesses++;

  //Return 0 to the gpr
  child->ctx.gpr[0] = 0;
  //Set gpr[0] to equal the child pid
  ctx->gpr[0] = child->pid;

  return;
}

void HL_exec(ctx_t* ctx){
  //Specify the new program counter from gpr[0]
  ctx->pc = ctx->gpr[0];
  return;
}

//End a process once the exit call has been made by making its status terminated
void HL_exit(ctx_t* ctx){
    pcb[current_pid()].status = STATUS_TERMINATED;
    pcb[current_pid()].priority = 0;
    pcb[current_pid()].age = 0;
    priority_schedule(ctx);
    return;
}


void hilevel_handler_rst( ctx_t* ctx              ) {
  /* Initialise the console pcb with the shared pointer pointing to the top of the general stack
  Also initialise all other PCB's with a status of termiated and the sp pointing to its specific position depending on its pid
   */
   memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );     // initialise 0-th PCB = CONSOLE
   pcb[ 0 ].pid      = 0;
   pcb[ 0 ].status   = STATUS_CREATED;
   pcb[ 0 ].ctx.cpsr = 0x50;
   pcb[ 0 ].ctx.pc   = ( uint32_t )( &main_console );
   pcb[ 0 ].ctx.sp   = ( uint32_t )( &tos_general  );
   pcb[ 0 ].priority = 1;
   pcb[ 0 ].age = 0;

   for (size_t i = 1; i < maximumPrograms; i++) {
     memset( &pcb[ i ], 0, sizeof( pcb_t ) );
     pcb[ i ].pid      = i;
     pcb[ i ].status   = STATUS_TERMINATED;
     pcb[ i ].ctx.cpsr = 0x50;
     pcb[ i ].ctx.pc   = ( uint32_t )( &main_console );
     pcb[ i ].ctx.sp   = giveStack(i);
     pcb[ i ].priority = 0;
     pcb[ i ].age = 0;
   }

  //Start running the console
  dispatch(ctx, NULL, &pcb[ 0 ]);
  numberprocesses = 1;

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
      break;
    }
    case SYS_FORK : { // 0x03 => fork()
      HL_fork(ctx);
      break;
    }

    case SYS_EXIT : { // 0x04 => exit(int x)
      HL_exit(ctx);
      break;
    }

    case SYS_EXEC : { // 0x05 => exec(const void* x)
      HL_exec(ctx);
      break;
    }
    default   : { // 0x?? => unknown/unsupported
      break;
    }
  }

  return;
}
