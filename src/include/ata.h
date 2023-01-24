#ifndef __ATA_H__
#define __ATA_H__

#include <stdint.h>

void ata_read(void *buf, uint32_t lba, uint8_t nsectors);
void ata_write(uint32_t lba, uint8_t nsectors, uint32_t *bytes);

void ata_read_bytes(void *buf, uint32_t off, uint32_t len);
void ata_write_bytes(void *buf, uint32_t off, uint32_t len);

#endif /* __ATA_H__ */
