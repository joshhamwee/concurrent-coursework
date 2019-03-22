/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __SYS_H
#define __SYS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "device.h"

/* As outlined in Section 4.3 and 4.4, especially Table 4.5, of
 *
 * http://infocenter.arm.com/help/topic/com.arm.doc.dui0417d/index.html
 *
 * the platform houses various configuration and control registers: these
 * allow a) control over and b) inspection of the state maintained by the
 * resources and devices it houses; some are rendered moot when placed in
 * an emulated context of course.
 */

typedef struct {
          RO uint32_t ID;               // 0x0000          : system identifier
          RO uint32_t USERSW;           // 0x0004          : user switch
          RW uint32_t LED;              // 0x0008          : user LED
          RW uint32_t OSC0;             // 0x000C          : oscillator  configuration
          RW uint32_t OSC1;             // 0x0010          : oscillator  configuration
          RW uint32_t OSC2;             // 0x0014          : oscillator  configuration
          RW uint32_t OSC3;             // 0x0018          : oscillator  configuration
          RW uint32_t OSC4;             // 0x001C          : oscillator  configuration
          RW uint32_t LOCK;             // 0x0020          : lock control
          RO uint32_t COUNTER_100HZ;    // 0x0024          : 100Hz counter
          RO RSVD( 0, 0x0028, 0x002C ); // 0x0028...0x002C : reserved
  union { RO uint32_t FLAGS;            // 0x0030          : general-purpose              flags
          WO uint32_t FLAGSSET;      }; // 0x0030          | general-purpose              flags set
          WO uint32_t FLAGSCLR;         // 0x0034          : general-purpose              flags clear
  union { RO uint32_t NVFLAGS;          // 0x0038          : general-purpose non-volatile flags 
          WO uint32_t NVFLAGSSET;    }; // 0x0038          | general-purpose non-volatile flags set
          WO uint32_t NVFLAGSCLR;       // 0x003C          : general-purpose non-volatile flags clear
          RW uint32_t RESETCTL;         // 0x0040          : software reset level
          RO RSVD( 1, 0x0044, 0x0047 ); // 0x0044...0x0047 : reserved
          RO uint32_t MCI;              // 0x0048          : MCI status
          RW uint32_t FLASH;            // 0x004C          : flash write protection
          RW uint32_t CLCD;             // 0x0050          : colour LCD power and multiplexing
          RO RSVD( 2, 0x0054, 0x0057 ); // 0x0054...0x0057 : reserved
          RO uint32_t CFGSW;            // 0x0058          : user switch configuration
          RO uint32_t COUNTER_24MHZ;    // 0x005C          : 24MHz counter
          RO uint32_t MISC;             // 0x0060          : miscellaneous
          RW uint32_t DMAPSR;           // 0x0064          : DMA mapping
          RO uint32_t PEX_STAT;         // 0x0068          : PCI Express status
          RW uint32_t PCI_STAT;         // 0x006C          : PCI         status
          RO RSVD( 3, 0x0070, 0x0073 ); // 0x0070...0x0073 : reserved
          RW uint32_t PLD_CTRL1;        // 0x0074          : PLD         configuration
          RW uint32_t PLD_CTRL2;        // 0x0078          : PLD         configuration
          RW uint32_t PLL_INIT;         // 0x007C          : PLL         configuration
          RO RSVD( 4, 0x0080, 0x0083 ); // 0x0080...0x0083 : reserved
          RO uint32_t PROCID0;          // 0x0084          : processor ID
          RO uint32_t PROCID1;          // 0x0088          : processor ID
          RW uint32_t OSCRESET0;        // 0x008C          : oscillator reset value
          RW uint32_t OSCRESET1;        // 0x0090          : oscillator reset value
          RW uint32_t OSCRESET2;        // 0x0094          : oscillator reset value
          RW uint32_t OSCRESET3;        // 0x0098          : oscillator reset value
          RW uint32_t OSCRESET4;        // 0x009A          : oscillator reset value
          RW uint32_t VOLTAGE_CTL0;     // 0x00A0          : voltate control/monitoring
          RW uint32_t VOLTAGE_CTL1;     // 0x00A4          : voltate control/monitoring
          RW uint32_t VOLTAGE_CTL2;     // 0x00A8          : voltate control/monitoring
          RW uint32_t VOLTAGE_CTL3;     // 0x00AA          : voltate control/monitoring
          RW uint32_t VOLTAGE_CTL4;     // 0x00AC          : voltate control/monitoring
          RW uint32_t VOLTAGE_CTL5;     // 0x00B0          : voltate control/monitoring
          RW uint32_t VOLTAGE_CTL6;     // 0x00B4          : voltate control/monitoring
          RW uint32_t VOLTAGE_CTL7;     // 0x00B8          : voltate control/monitoring
          RW uint32_t VOLTAGE_CTL8;     // 0x00BC          : voltate control/monitoring
          RO uint32_t TEST_OSC0;        // 0x00C0          : oscillator driven counter
          RO uint32_t TEST_OSC1;        // 0x00C4          : oscillator driven counter
          RO uint32_t TEST_OSC2;        // 0x00C8          : oscillator driven counter
          RO uint32_t TEST_OSC3;        // 0x00CC          : oscillator driven counter
          RO uint32_t TEST_OSC4;        // 0x00D0          : oscillator driven counter
          RW uint32_t OSC5;             // 0x00D4          : oscillator  configuration
          RW uint32_t OSC6;             // 0x00D8          : oscillator  configuration
          RW uint32_t OSCRESET5;        // 0x00DC          : oscillator reset value
          RW uint32_t OSCRESET6;        // 0x00E0          : oscillator reset value
          RO uint32_t TEST_OSC5;        // 0x00E4          : oscillator driven counter
          RO uint32_t TEST_OSC6;        // 0x00E8          : oscillator driven counter
          RW uint32_t OSC7;             // 0x00EC          : oscillator  configuration
          RW uint32_t OSCRESET7;        // 0x00F0          : oscillator reset value
          RO uint32_t TEST_OSC7;        // 0x00F4          : oscillator driven counter
          RW uint32_t DEBUG;            // 0x00F8          : debug unit  configuration
          RW uint32_t TESTMODE;         // 0x00FC          : test mode   configuration
          RW uint32_t PLL_RESET;        // 0x0100          : PLL        reset value
} SYSCONF_t;

/* Per Table 4.2 (for example: the information is in several places) of
 * 
 * http://infocenter.arm.com/help/topic/com.arm.doc.dui0417d/index.html
 * 
 * we know the registers are mapped to fixed addresses in memory, so we
 * can just define a (structured) pointer to each one to support access.
 */

extern SYSCONF_t* SYSCONF;

extern  uint32_t* SYSCTRL0;
extern  uint32_t* SYSCTRL1;

#endif
