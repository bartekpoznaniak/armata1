#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>



#include "stm32f1xx_hal.h"
extern UART_HandleTypeDef huart2;

// I zastąp _write tym:
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { errno = EINVAL; return -1; }
void _exit(int status) { while(1) {} }
int _close(int file) { return -1; }
int _fstat(int file, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
int _isatty(int file) { return 1; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _read(int file, char *ptr, int len) { return 0; }
//int _write(int file, char *ptr, int len) { return len; }
int _sbrk(int incr) {
    extern char end asm("end");
    static char *heap_end;
    char *prev_heap_end;
    if (heap_end == 0) heap_end = &end;
    prev_heap_end = heap_end;
    heap_end += incr;
    return (int)prev_heap_end;
}
