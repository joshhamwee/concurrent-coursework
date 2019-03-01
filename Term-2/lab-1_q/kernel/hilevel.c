/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

void hilevel_handler_rst() {
  char* x = "hello world\n";

  while( 1 ) {
    for( int i = 0; i < 12; i++ ) {
      PL011_putc( UART0, x[ i ], true );
    }
  }

  return;
}
