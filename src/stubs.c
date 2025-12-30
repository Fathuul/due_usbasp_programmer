/* src/stubs.c - System Stubs & Exception Handlers */
#include <sys/stat.h>
#include <stdint.h>
#include "sam3x8e.h"
#include "uart_dbg.h"

// --- 1. Syscalls (for Newlib) ---
// Retarget _write() to debug UART

int _close(int file) { (void)file; return -1; }

int _fstat(int file, struct stat *st)
{
  (void)file;
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file)
{
  (void)file;
  return 1;
}

int _lseek(int file, int ptr, int dir)
{
  (void)file;
  (void)ptr;
  (void)dir;
  return 0;
}

int _read(int file, char *ptr, int len)
{
  (void)file;
  (void)ptr;
  (void)len;
  return 0;
}

int _write(int file, char *ptr, int len)
{
  (void)file;

  if (ptr && len > 0) {
    uart_dbg_write(ptr, (uint32_t)len);
  }

  return len;
}

int _getpid(void) { return 1; }

int _kill(int pid, int sig)
{
  (void)pid;
  (void)sig;
  return -1;
}

void _exit(int status)
{
  (void)status;
  while (1) { }
}

// --- Memory management (malloc support) ---

extern int _end;    // Defined in linker script
extern int _estack; // Defined in linker script

extern caddr_t _sbrk(int incr)
{
  static unsigned char *heap = NULL;
  unsigned char *prev_heap;

  if (heap == NULL) {
    heap = (unsigned char *)&_end;
  }

  prev_heap = heap;
  heap += incr;

  return (caddr_t) prev_heap;
}

// --- 2. Exception Handlers ---

void Dummy_Handler(void)
{
  while (1) { }
}

// Core exceptions
void NMI_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void HardFault_Handler  ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void MemManage_Handler  ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void BusFault_Handler   ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void UsageFault_Handler ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void SVC_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void DebugMon_Handler   ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void PendSV_Handler     ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void SysTick_Handler    ( void ) __attribute__ ((weak, alias("Dummy_Handler")));

// Peripherals
void SUPC_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void RSTC_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void RTC_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void RTT_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void WDT_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void PMC_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void EFC0_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void EFC1_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void UART_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void SMC_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void SDRAMC_Handler     ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void PIOA_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void PIOB_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void PIOC_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void PIOD_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void USART0_Handler     ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void USART1_Handler     ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void USART2_Handler     ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void USART3_Handler     ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void HSMCI_Handler      ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TWI0_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TWI1_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void SPI0_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void SSC_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TC0_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TC1_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TC2_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TC3_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TC4_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TC5_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TC6_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TC7_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TC8_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void PWM_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void ADC_Handler        ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void DACC_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void DMAC_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TRNG_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void EMAC_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void CAN0_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void CAN1_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));

// NOTE: UOTGHS_Handler is defined elsewhere (USB), no alias here
