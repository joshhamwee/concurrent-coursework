/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

void hilevel_handler_rst() {
  /* Configure the mechanism for interrupt handling by
   *
   * - configuring UART st. an interrupt is raised every time a byte is
   *   subsequently received,
   * - configuring GIC st. the selected interrupts are forwarded to the 
   *   processor via the IRQ interrupt signal, then
   * - enabling IRQ interrupts.
   */

  UART0->IMSC       |= 0x00000010; // enable UART    (Rx) interrupt
  UART0->CR          = 0x00000301; // enable UART (Tx+Rx)

  GICC0->PMR         = 0x000000F0; // unmask all          interrupts
  GICD0->ISENABLER1 |= 0x00001000; // enable UART    (Rx) interrupt
  GICC0->CTLR        = 0x00000001; // enable GIC interface
  GICD0->CTLR        = 0x00000001; // enable GIC distributor

  int_enable_irq();

  /* Force execution into an infinite loop, each iteration of which will
   *
   * - delay for some period of time, which is realised by executing a
   *   large, fixed number of nop instructions in an inner loop, then
   * - execute a supervisor call (i.e., svc) instruction, thus raising
   *   a software-interrupt (i.e., a trap or system call).
   */

  while( 1 ) {
    for( int i = 0; i < 0x20000000; i++ ) {
      asm volatile( "nop" );
    }

    asm volatile( "svc 0" );
  }

  return;
}

void hilevel_handler_irq() {
  // Step 2: read  the interrupt identifier so we know the source.

  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.

  if( id == GIC_SOURCE_UART0 ) {
    uint8_t x = PL011_getc( UART0, true );

    PL011_putc( UART0, 'K',                      true ); 
    PL011_putc( UART0, '<',                      true ); 
    PL011_putc( UART0, itox( ( x >> 4 ) & 0xF ), true ); 
    PL011_putc( UART0, itox( ( x >> 0 ) & 0xF ), true ); 
    PL011_putc( UART0, '>',                      true ); 

    UART0->ICR = 0x10;
  }

  // Step 5: write the interrupt identifier to signal we're done.

  GICC0->EOIR = id;

  return;
}

void hilevel_handler_svc() {
  /* Each time execution reaches this point, we are tasked with handling
   * a supervisor call (aka. software interrupt, i.e., a trap or system 
   * call).
   */

  PL011_putc( UART0, 'T', true );  

  return;
}

