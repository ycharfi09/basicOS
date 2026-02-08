#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <stdbool.h>

/* ATA registers */
#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_SECONDARY_IO    0x170
#define ATA_SECONDARY_CTRL  0x376

/* ATA commands */
#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_WRITE_PIO   0x30
#define ATA_CMD_IDENTIFY    0xEC

/* ATA status bits */
#define ATA_SR_BSY   0x80   /* Busy */
#define ATA_SR_DRDY  0x40   /* Drive ready */
#define ATA_SR_DF    0x20   /* Drive write fault */
#define ATA_SR_DSC   0x10   /* Drive seek complete */
#define ATA_SR_DRQ   0x08   /* Data request ready */
#define ATA_SR_CORR  0x04   /* Corrected data */
#define ATA_SR_IDX   0x02   /* Index */
#define ATA_SR_ERR   0x01   /* Error */

/* Initialize ATA driver */
void ata_init(void);

/* Read sectors from disk */
bool ata_read_sectors(uint32_t lba, uint8_t sector_count, uint8_t *buffer);

/* Write sectors to disk */
bool ata_write_sectors(uint32_t lba, uint8_t sector_count, const uint8_t *buffer);

/* Check if ATA drive is present */
bool ata_drive_present(void);

#endif /* ATA_H */
