#include "ata.h"
#include <stdint.h>
#include <stdbool.h>

/* I/O port operations */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

/* Current ATA configuration */
static uint16_t ata_io_base = ATA_PRIMARY_IO;
static bool drive_present = false;

/* Wait for ATA drive to be ready */
static bool ata_wait_ready(void) {
    uint8_t status;
    int timeout = 10000;
    
    while (timeout-- > 0) {
        status = inb(ata_io_base + 7);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)) {
            return true;
        }
        io_wait();
    }
    return false;
}

/* Wait for data request */
static bool ata_wait_drq(void) {
    uint8_t status;
    int timeout = 10000;
    
    while (timeout-- > 0) {
        status = inb(ata_io_base + 7);
        if (status & ATA_SR_DRQ) {
            return true;
        }
        if (status & ATA_SR_ERR) {
            return false;
        }
        io_wait();
    }
    return false;
}

/* Initialize ATA driver */
void ata_init(void) {
    /* Try to identify the drive */
    if (!ata_wait_ready()) {
        drive_present = false;
        return;
    }
    
    /* Select master drive */
    outb(ata_io_base + 6, 0xA0);
    io_wait();
    
    /* Send IDENTIFY command */
    outb(ata_io_base + 7, ATA_CMD_IDENTIFY);
    io_wait();
    
    /* Check if drive exists */
    uint8_t status = inb(ata_io_base + 7);
    if (status == 0) {
        drive_present = false;
        return;
    }
    
    /* Wait for response */
    if (!ata_wait_drq()) {
        drive_present = false;
        return;
    }
    
    /* Read identification data (256 words) */
    for (int i = 0; i < 256; i++) {
        (void)inb(ata_io_base + 0);
        (void)inb(ata_io_base + 0);
    }
    
    drive_present = true;
}

/* Check if ATA drive is present */
bool ata_drive_present(void) {
    return drive_present;
}

/* Read sectors from disk */
bool ata_read_sectors(uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
    if (!drive_present || sector_count == 0) {
        return false;
    }
    
    /* Wait for drive to be ready */
    if (!ata_wait_ready()) {
        return false;
    }
    
    /* Select drive and LBA mode */
    outb(ata_io_base + 6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ata_io_base + 2, sector_count);
    outb(ata_io_base + 3, (uint8_t)(lba));
    outb(ata_io_base + 4, (uint8_t)(lba >> 8));
    outb(ata_io_base + 5, (uint8_t)(lba >> 16));
    outb(ata_io_base + 7, ATA_CMD_READ_PIO);
    
    /* Read sectors */
    for (uint8_t i = 0; i < sector_count; i++) {
        if (!ata_wait_drq()) {
            return false;
        }
        
        /* Read 256 words (512 bytes) */
        uint16_t *buf16 = (uint16_t *)(buffer + i * 512);
        for (int j = 0; j < 256; j++) {
            buf16[j] = inb(ata_io_base + 0) | (inb(ata_io_base + 0) << 8);
        }
        
        /* Check for errors */
        uint8_t status = inb(ata_io_base + 7);
        if (status & ATA_SR_ERR) {
            return false;
        }
    }
    
    return true;
}

/* Write sectors to disk */
bool ata_write_sectors(uint32_t lba, uint8_t sector_count, const uint8_t *buffer) {
    if (!drive_present || sector_count == 0) {
        return false;
    }
    
    /* Wait for drive to be ready */
    if (!ata_wait_ready()) {
        return false;
    }
    
    /* Select drive and LBA mode */
    outb(ata_io_base + 6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ata_io_base + 2, sector_count);
    outb(ata_io_base + 3, (uint8_t)(lba));
    outb(ata_io_base + 4, (uint8_t)(lba >> 8));
    outb(ata_io_base + 5, (uint8_t)(lba >> 16));
    outb(ata_io_base + 7, ATA_CMD_WRITE_PIO);
    
    /* Write sectors */
    for (uint8_t i = 0; i < sector_count; i++) {
        if (!ata_wait_drq()) {
            return false;
        }
        
        /* Write 256 words (512 bytes) */
        const uint16_t *buf16 = (const uint16_t *)(buffer + i * 512);
        for (int j = 0; j < 256; j++) {
            outb(ata_io_base + 0, buf16[j] & 0xFF);
            outb(ata_io_base + 0, buf16[j] >> 8);
        }
        
        /* Flush cache */
        outb(ata_io_base + 7, 0xE7);
        
        /* Check for errors */
        uint8_t status = inb(ata_io_base + 7);
        if (status & ATA_SR_ERR) {
            return false;
        }
    }
    
    return true;
}
