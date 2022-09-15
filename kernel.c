#include <stddef.h>
#include <stdint.h>
#include <float.h>

#include "uart.h"
#include "util.h"
#include "mailbox.h"

#define AARCH64
 
#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
 
void execute_cmd (unsigned char* buf) {
    if (strcmp(buf, "help")) {
        println ("There's no help out here");
    } else if (strcmp(buf, "count")) {
        for (int i = 0; i < 100; i++) {
            print ("Number: ");
            printint (i);
            newln();
        }
    }  else {
        println ("Unrecognised");
    }
}

void clear_buf (unsigned char* buf, size_t n) {
    for (size_t i = 0; i <= n; i++) buf[i] = 0x0;
}

#pragma pack(1)
typedef struct FRAMEBUFFER {
    uint32_t physical_width;
    uint32_t physical_height;
    uint32_t virtual_width;
    uint32_t virtual_height;
    uint32_t pitch;
    uint32_t depth;
    uint32_t virtual_x_offset;
    uint32_t virtual_y_offset;
    uint32_t framebuffer_address;
    uint32_t framebuffer_size;
} FRAMEBUFFER;


void dump_memory (void *start, size_t size) {
    for (size_t i = 0; i < size; i++) {
        printhex (*(char *)start);
        newln();
    }
}

#ifdef AARCH64
// arguments for AArch64
void kernel_main(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3)
#else
// arguments for AArch32
void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
#endif
{
	// initialize UART for Raspi3
	uart_init(3);
	uart_puts("Hello, kernel World!\r\n");

    unsigned char cmd_buf[65];
    for (uint8_t i = 0; i < 65; i++) {
        cmd_buf[i] = 0x0;
    }
    size_t cmd_buf_p = 0;


    uint32_t FRAMEBUFFER_START[40];

    void *head = (void *)FRAMEBUFFER_START;
    FRAMEBUFFER *buf = (FRAMEBUFFER *)head;

    *buf = (FRAMEBUFFER){
        640,
        480,
        640,
        480,
        0,
        24,
        0,
        0,
        0,
        0
    };

    mailbox_write (MBOX_CH_FRAMEBUFFER, ((uint32_t)FRAMEBUFFER_START >> 4));

    println ("mailbox written");

    uint32_t result = mailbox_read ();

    println ("mailbox read");

    printhex (result);
    newln();
    printhex (buf->framebuffer_address);
    newln();
    printint (buf->framebuffer_size);
    newln();
    printint (buf->physical_width);
    newln();
    printint (buf->physical_height);
    newln();
    printint (buf->pitch);
    newln();
    printint (buf->depth);
    newln();

    uint32_t *framebuf_addr = (char*)((buf->framebuffer_address) & 0x3FFFFFFF);

    for (int y = 0; y < buf->virtual_height; y++) {
        for (int x = 0; x < buf->virtual_width; x++) {
            if (x % 4) {
                framebuf_addr[((y*buf->virtual_width)+x)*3] = 0xFF;
            }
            if (x % 3) {
                framebuf_addr[(((y*buf->virtual_width)+x)*3) + 1] = 0xFF;
            }
            if (x % 2) {
                framebuf_addr[(((y*buf->virtual_width)+x)*3)+2] = 0xFF;
            }
        }
    }
    // for (int i = 0; i < 921600; i+=4) {
    //     framebuf_addr[i] = 0xFF;
    // }

	while (1) {
        unsigned char c = uart_getc();
        if (c == '\r') {
            uart_putc('\n');
            execute_cmd (cmd_buf);
            clear_buf (cmd_buf, cmd_buf_p);
            cmd_buf_p = 0;
        } else if (c == '\b' && cmd_buf_p > 0) {
            cmd_buf_p--;
            cmd_buf[cmd_buf_p] = 0x0;
        } else if (cmd_buf_p < 65) {
            cmd_buf[cmd_buf_p] = c;
            cmd_buf_p++;
        }

		uart_putc(c);
    }



}