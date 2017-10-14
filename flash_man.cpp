/************************************************************************//**
 * \file
 *
 * \brief Flash Manager.
 *
 * Handles basic operations on flash chips (Program/Read/Erase) using MDMA
 * interface.
 *
 * \author doragasu
 * \date   2017
 ****************************************************************************/
#include <QApplication>
#include <stdlib.h>
#include "flash_man.h"
#include "util.h"
#include "commands.h"

/********************************************************************//**
 * Program a file to the flash chip.
 *
 * \param[in] filename  File name to program.
 * \param[in] autoErase Erase the flash range where the file will be
 *            flashed.
 * \param[in] start     Word memory addressh where the file will be
 *            programmed.
 * \param[in] len       Number of words to write to the flash.
 *
 * \return A pointer to a buffer containing the file to flash (byte
 * swapped) or NULL if the operation fails.
 *
 * \warning The user is responsible of freeing the buffer calling
 * BufFree() when its contents are not needed anymore.
 ************************************************************************/
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
		emit StatusChanged("Auto erasing");
		QApplication::processEvents();
		DelayMs(1);
		if (MDMA_range_erase(*start, *len)) {
			free(writeBuf);
			return NULL;
		}
	}

	emit RangeChanged(0, *len);
	emit ValueChanged(0);
	emit StatusChanged("Programming");
	QApplication::processEvents();

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
		QApplication::processEvents();
	}
	emit ValueChanged(i);
	emit StatusChanged("Done!");
	QApplication::processEvents();
	return writeBuf;
}

/********************************************************************//**
 * Read a memory range from the flash chip.
 *
 * \param[in] start Word memory address to start reading from.
 * \param[in] len   Number of words to read from flash.
 *
 * \return A pointer to the buffer containing the data read from the
 * flash, or NULL if the read operation has failed.
 *
 * \warning The user is responsible of freeing the buffer calling
 * BufFree() when its contents are not needed anymore.
 ************************************************************************/
uint16_t *FlashMan::Read(uint32_t start, uint32_t len) {
	uint16_t *readBuf;
	int toRead;
	uint32_t addr;
	uint32_t i;

	emit RangeChanged(0, len);
	emit ValueChanged(0);
	emit StatusChanged("Reading");
	QApplication::processEvents();

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
		QApplication::processEvents();
	}
	emit ValueChanged(i);
	emit StatusChanged("Done");
	QApplication::processEvents();
	return readBuf;
}

/********************************************************************//**
 * Erases a memory range from the flash chip.
 *
 * \param[in] start Word memory address of the beginning of the range
 *            to erase.
 * \param[in] len   Length (in words) of the range to erase.
 *
 * \return 0 on success, non-zero if erase operation fails.
 ************************************************************************/
int FlashMan::RangeErase(uint32_t start, uint32_t len) {
	if (MDMA_range_erase(start, len)) return -1;

	return 0;
}

/********************************************************************//**
 * Issues a complete chip erase command to the flash chip.
 *
 * \return 0 on success, non-zero if erase operation fails.
 ************************************************************************/
int FlashMan::FullErase(void) {
	if (MDMA_cart_erase()) return -1;

	return 0;
}

/********************************************************************//**
 * Frees a buffer previously allocated by Program() or Read().
 *
 * \param[in] buf The address of the buffer to free.
 ************************************************************************/
void FlashMan::BufFree(uint16_t *buf) {
	free(buf);
}

/********************************************************************//**
 * Obtains the flash chip 16-bit Manufacturer ID code.
 *
 * \param[out] manId The 16-bit Manufacturer ID of the flash chip.
 *
 * \return 0 on success, non-zero on error.
 ************************************************************************/
uint16_t FlashMan::ManIdGet(uint16_t *manId) {
	return MDMA_manId_get(manId);
}

/********************************************************************//**
 * Obtains the 3 flash chip 16-bit Device ID codes.
 *
 * \param[out] devIds The 3 16-bit Device IDs of the flash chip.
 *
 * \return 0 on success, non-zero on error.
 ************************************************************************/
uint16_t FlashMan::DevIdGet(uint16_t devIds[3]) {
	return MDMA_devId_get(devIds);
}

/********************************************************************//**
 * Enters DFU bootloader mode.
 *
 * \return 0 on success, non-zero on error.
 ************************************************************************/
uint16_t FlashMan::DfuBootloader(void) {
	return MDMA_bootloader();
}

