#include <stdlib.h>
#include "flash_man.h"
#include "util.h"
#include "commands.h"

uint16_t *FlashMan::Program(const char filename[], bool autoErase,
		uint32_t *start, uint32_t *len) {
    FILE *rom;
	uint16_t *writeBuf;
	uint32_t addr;
	int toWrite;
	uint32_t i;

	// Open the file to flash
	if (!(rom = fopen(filename, "rb"))) return NULL;

	// Obtain length if not specified
	if (!(*len)) {
	    fseek(rom, 0, SEEK_END);
	    *len = ftell(rom)>>1;
	    fseek(rom, 0, SEEK_SET);
	}

    writeBuf = (uint16_t*)malloc(*len<<1);
	if (!writeBuf) {
		fclose(rom);
		return NULL;
	}
    fread(writeBuf, (*len)<<1, 1, rom);
	fclose(rom);
   	// Do byte swaps
   	for (i = 0; i < (*len); i++) ByteSwapWord(writeBuf[i]);

	// If requested, perform auto-erase
	if (autoErase) {
		emit StatusChanged("Auto Erasing...");
		if (MDMA_range_erase(*start, *len)) {
			free(writeBuf);
			return NULL;
		}
	}

	emit RangeChanged(0, *len);
	emit ValueChanged(0);
	emit StatusChanged("Programming");

	for (i = 0, addr = *start; i < (*len);) {
		toWrite = MIN(65536>>1, (*len) - i);
		if (MDMA_write(toWrite, addr, writeBuf + i)) {
			free(writeBuf);
			fclose(rom);
			return NULL;
		}
		// Update vars and draw progress bar
		i += toWrite;
		addr += toWrite;
		emit ValueChanged(i);
	}
	emit ValueChanged(i);
	emit StatusChanged("Done");
	return writeBuf;
}

uint16_t *FlashMan::Read(uint32_t start, uint32_t len) {
	uint16_t *readBuf;
	int toRead;
	uint32_t addr;
	uint32_t i;

	emit RangeChanged(0, len);
	emit ValueChanged(0);
	emit StatusChanged("Reading");

	readBuf = (uint16_t*)malloc(len<<1);
	if (!readBuf) {
		return NULL;
	}

	for (i = 0, addr = start; i < len;) {
		toRead = MIN(65536>>1, len - i);
		if (MDMA_read(toRead, addr, readBuf + i)) {
			free(readBuf);
			return NULL;
		}
		// Update vars and draw progress bar
		i += toRead;
		addr += toRead;
		emit ValueChanged(i);
	}
	emit ValueChanged(i);
	emit StatusChanged("Done");
	return readBuf;
}

int FlashMan::RangeErase(uint32_t start, uint32_t len) {
	return 0;
}

int FlashMan::FullErase(void) {
	return 0;
}

int FlashMan::Verify(uint16_t *readBuf, uint16_t *writeBuf, uint32_t len) {
	return 0;
}

void FlashMan::BufFree(uint16_t *buf) {
	free(buf);
}

