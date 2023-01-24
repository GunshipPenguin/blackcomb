#include "io.h"
#include "util.h"
#include "kmalloc.h"
#include "string.h"

#define SECTOR_SIZE 512

#define ATA_DATA_REG 0x1F0
#define ATA_ERR_FEAT_REG 0x1F1
#define ATA_SECTOR_COUNT_REG 0x1F2
#define ATA_SECTOR_NUMBER_REG 0x1F3
#define ATA_CYLINDER_LOW_REG 0x1F4
#define ATA_CYLINDER_HIGH_REG 0x1F5
#define ATA_DRIVE_HEAD_REG 0x1F6
#define ATA_STATUS_COMMAND_REG 0x1F7

#define STATUS_BSY 0x80
#define STATUS_RDY 0x40
#define STATUS_DF 0x20
#define STATUS_SRV 0x10
#define STATUS_DRQ 0x08
#define STATUS_CORR 0x04
#define STATUS_IDX 0x04
#define STATUS_ERR 0x01

#define COMMAND_WRITE 0x30
#define COMMAND_READ 0x20

static void wait_ready()
{
    while (1) {
        uint8_t status = inb(ATA_STATUS_COMMAND_REG);
        if ((status & STATUS_RDY) && !(status & STATUS_BSY))
            break;
    }
}

void load_lba_nsectors(uint32_t lba, uint8_t nsectors)
{
    outb(ATA_DRIVE_HEAD_REG, 0xE0 | ((lba >> 24) & 0xF));
    outb(ATA_SECTOR_COUNT_REG, nsectors);
    outb(ATA_SECTOR_NUMBER_REG, lba & 0xF);
    outb(ATA_CYLINDER_LOW_REG, (lba >> 8) & 0xF);
    outb(ATA_CYLINDER_HIGH_REG, (lba >> 16) & 0xF);
}

void ata_read(void *buf, uint32_t lba, uint8_t nsectors)
{
    wait_ready();
    load_lba_nsectors(lba, nsectors);
    outb(ATA_STATUS_COMMAND_REG, COMMAND_READ);

    for (int j = 0; j < nsectors; j++) {
        wait_ready();
        for (int i = 0; i < 256; i++)
            ((uint16_t *)buf)[i + (j * 256)] = inw(ATA_DATA_REG);
    }
}

void ata_write(void *buf, uint32_t lba, uint8_t nsectors)
{
    wait_ready();
    load_lba_nsectors(lba, nsectors);
    outb(ATA_STATUS_COMMAND_REG, COMMAND_WRITE);

    for (int j = 0; j < nsectors; j++) {
        wait_ready();
        for (int i = 0; i < 256; i++) {
            outw(ATA_DATA_REG, ((uint16_t *)buf)[i + (j * 256)]);
        }
    }
}

void ata_read_bytes(void *buf, uint32_t off, uint32_t len)
{
    uint32_t off_lba = off / SECTOR_SIZE;
    /* ceil(len / SECTOR_SIZE) */
    uint32_t nsectors = (len + SECTOR_SIZE - 1) / SECTOR_SIZE;

    /* TODO: This alloc is unncessary, get rid of it for perf reasons */
    void *readbuf = kmalloc(nsectors * SECTOR_SIZE);
    if (!buf)
        panic("out of memory");

    ata_read(readbuf, off_lba, nsectors);
    memcpy(buf, readbuf, len);
    free(readbuf);
}

void ata_write_bytes(void *buf, uint32_t off, uint32_t len)
{
    uint32_t off_lba = off / SECTOR_SIZE;
    /* ceil(len / SECTOR_SIZE) */
    uint32_t nsectors = (len + SECTOR_SIZE - 1) / SECTOR_SIZE;

    /* TODO: This alloc is unncessary, get rid of it for perf reasons */
    void *readbuf = kmalloc(nsectors * SECTOR_SIZE);
    if (!buf)
        panic("out of memory");

    ata_write(readbuf, off_lba, nsectors);
    memcpy(buf, readbuf, len);
    free(readbuf);
}
