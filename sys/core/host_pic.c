/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2018                           |
 * |                                                            |
 * | Host Programmable Interrupt Controller (i8259)             |
 * | initialization and interrupt remapping driver              |
 * |                                                            |
 * | All IRQs are remapped to the usable [0x20..0x2F] range     |
 * | to eliminate collision with the CPU exception vector       |
 * |                                                            |
 * | Based on wiki.osdev.org/wiki/8259_PIC                      |
 * +------------------------------------------------------------+
*/

#include <types.h>
#include <system.h>

/* Master PIC controller */
#define PIC0_CMD_PORT     0x20
#define PIC0_DATA_PORT    0x21

/* Slave PIC controller */
#define PIC1_CMD_PORT     0xA0
#define PIC1_DATA_PORT    0xA1

#define PIC_EOI           0x20        /* End of interrupt signal */

#define ICW1_ICW4         0x01        /*  ICW4 (not) needed */
#define ICW1_SINGLE       0x02        /*  Single (cascade) mode */
#define ICW1_INTERVAL4    0x04        /*  Call address interval 4 (8) */
#define ICW1_LEVEL        0x08        /*  Level triggered (edge) mode */
#define ICW1_INIT         0x10        /*  Initialization - required! */
 
#define ICW4_8086         0x01        /*  8086/88 (MCS-80/85) mode */
#define ICW4_AUTO         0x02        /*  Auto (normal) EOI */
#define ICW4_BUF_SLAVE    0x08        /*  Buffered mode/slave */
#define ICW4_BUF_MASTER   0x0C        /*  Buffered mode/master */
#define ICW4_SFNM         0x10        /*  Special fully nested (not) */

void pic_disable(void)
{
    outb(PIC1_CMD_PORT, 0xFF);
    outb(PIC0_DATA_PORT, 0xFF);
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8) {
        outb(PIC1_CMD_PORT, PIC_EOI);
    }
    outb(PIC0_CMD_PORT, PIC_EOI);
}

int pic_init(int offset1, int offset2)
{
    uint8_t mask0, mask1;

    /* Save the current masks */
    mask0 = inb(PIC0_DATA_PORT);
    mask1 = inb(PIC1_DATA_PORT);

    /* Cascade mode */
    outb(PIC0_CMD_PORT, ICW1_INIT+ICW1_ICW4);
    io_wait();
    outb(PIC1_CMD_PORT, ICW1_INIT+ICW1_ICW4);
    io_wait();

    /* ICW2: Master PIC vector offset */
    outb(PIC0_DATA_PORT, offset1);
    io_wait();
    /* ICW2: Slave PIC vector offset */
    outb(PIC1_DATA_PORT, offset2); 
    io_wait();
    /* ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100) */
    outb(PIC0_DATA_PORT, 4);    
    io_wait();
    /* ICW3: tell Slave PIC its cascade identity (0000 0010) */
    outb(PIC1_DATA_PORT, 2);
    io_wait();

    outb(PIC0_DATA_PORT, ICW4_8086);
    io_wait();
    outb(PIC1_DATA_PORT, ICW4_8086);
    io_wait();

    /* Restore saved masks */
    outb(PIC0_DATA_PORT, mask0);
    outb(PIC1_DATA_PORT, mask1);
    return 0;
}
