#include <stdlib.h>
#include "flash_man.h"
#include "util.h"
#include "commands.h"

FlashMan::FlashMan() {
	maxMinCb = NULL;
	activityCb = NULL;
}

uint16_t *FlashMan::Program(const char filename[], bool autoErase,
		uint32_t start, uint32_t len) {
    FILE *rom;
	u16 *writeBuf;
	uint32_t addr;
	int toWrite;
	uint32_t i;

	// Open the file to flash
	if (!(rom = fopen(filename, "rb"))) return NULL;

	// Obtain length if not specified
	if (!len) {
	    fseek(rom, 0, SEEK_END);
	    len = ftell(rom)>>1;
	    fseek(rom, 0, SEEK_SET);
	}

    writeBuf = (uint16_t*)malloc(len<<1);
	if (!writeBuf) {
		fclose(rom);
		return NULL;
	}
    fread(writeBuf, len<<1, 1, rom);
	fclose(rom);
   	// Do byte swaps
   	for (i = 0; i < len; i++) ByteSwapWord(writeBuf[i]);

	// If requested, perform auto-erase
	if (autoErase) {
		if (activityCb) activityCb(FLASHMAN_ERASE, 0);
		if (MDMA_range_erase(start, len)) {
			free(writeBuf);
			return NULL;
		}
	}

	if (maxMinCb) maxMinCb(0, len);
	if (activityCb) activityCb(FLASHMAN_PROGRAM, 0);

	for (i = 0, addr = start; i < len;) {
		toWrite = MIN(65536>>1, len - i);
		if (MDMA_write(toWrite, addr, writeBuf + i)) {
			free(writeBuf);
			fclose(rom);
			return NULL;
		}
		// Update vars and draw progress bar
		i += toWrite;
		addr += toWrite;
		if (activityCb) activityCb(FLASHMAN_PROGRAM, i);
	}
	if (activityCb) activityCb(FLASHMAN_DONE, i);
	return writeBuf;
}

uint16_t *FlashMan::Read(const char filename[], uint32_t start,
		uint32_t len) {
	return NULL;
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

void FlashMan::SetMaxMinCallback(MaxMinCb maxMinCb) {
	this->maxMinCb = maxMinCb;
}

void FlashMan::SetActivityCallback(ActivityCb activityCb) {
	this->activityCb = activityCb;
}

void FlashMan::BufFree(uint16_t *buf) {
	free(buf);
}

