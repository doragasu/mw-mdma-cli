/************************************************************************//**
 * \file
 * \brief Reads ESP8266 firmware blobs and sends to the MeGaWiFi programmer
 * the required commands to flash the blobs.
 *
 * Implementation based on the information and code of the original
 * esptool: https://github.com/themadinventor/esptool
 *
 * \author Jesús Alonso (doragasu)
 * \date   2016
 *
 * \warning Module is NOT reentrant!
 ****************************************************************************/

#include "esp-prog.h"
#include "commands.h"
#include "util.h"
#include <sys/stat.h>
#include "progbar.h"

// Buffer with a flash block
static EpBuf buf;

void EpInit(void) {
}

static uint32_t EpCsum(uint8_t *data, uint16_t len) {
	uint8_t csum = EP_CSUM_MAGIC;
	uint16_t i;

	for (i = 0; i < len; i++) csum ^= data[i];

	return csum;
}

static int EpSendCmd(EpCmdOp cmd, uint8_t data[], uint16_t len, uint32_t csum) {
	int ret;
	uint16_t i;

	if (len > EP_FLASH_PKT_LEN) return -1;

	buf.req.hdr.dir = EP_DIR_REQ;
	buf.req.hdr.cmd = cmd;
	/// \warning check if length must be in bytes or dwords
	buf.req.hdr.bLen = len;
	buf.req.hdr.csum = csum;
	// Copy data
	for (i = 0; i < len; i++) buf.req.data[i] = data[i];
	// Depending on the command, use the appropiate method to send data
	if ((EP_OP_RAM_DOWNLOAD_DATA == cmd) ||
			(EP_OP_FLASH_DOWNLOAD_DATA) == cmd) {
		ret = MDMA_WiFiCmdLong(buf.data, len + sizeof(EpReqHdr), buf.data);
	} else {
		ret = MDMA_WiFiCmd(buf.data, len + sizeof(EpReqHdr), buf.data);
	}
	if (0 > ret) return -1;
	
	return buf.resp.hdr.resp;
}

/// Starts download process (deletes flash and prepares for data download).
static int EpDownloadStart(size_t fLen, uint32_t addr) {
	uint32_t data[4];
	// Command requires total size (including padding), number of blocks,
	// block size and offset (address).
	// First compute number of blocks (rounding up!).
	data[1] = (fLen + EP_FLASH_SECT_LEN - 1) / EP_FLASH_SECT_LEN;
	// Now compute total size and fill block size and address.
	data[0] = data[1] * EP_FLASH_SECT_LEN;
	data[2] = EP_FLASH_SECT_LEN;
	data[3] = addr;

	// Send prepared command
	return EpSendCmd(EP_OP_FLASH_DOWNLOAD_START, (uint8_t*)data, sizeof(data),
			0);
	
	return 0;
}

// TODO: This function is a mess and should be split
int EpBlobFlash(char file[], uint32_t addr, Flags f) {
	size_t fLen;
	size_t totalLen;
	uint16_t nSect;
	uint16_t seq;
	uint32_t readed;
	uint32_t pos;
	uint16_t toRead;
	uint16_t csum;
	size_t flashed;
	struct stat st;
	FILE *fi = NULL;
	uint8_t *fBuf = NULL;
	EpBlobHdr *blobHdr;
	int retVal = 0;
	uint32_t reboot;
	// Address string, e.g.: 0x12345678
	char addrStr[11];
	unsigned int i;

	// Get file length
	if (stat(file, &st)) {
		perror(file);
		return -1;
	}
	fLen = st.st_size;
	
	// Allocate memory for each flash packet along with its packet header
	// and read the file. Round length up to an EP_FLASH_SECT multiple
	totalLen = (fLen + EP_FLASH_SECT_LEN - 1) & (~(EP_FLASH_SECT_LEN - 1));
	nSect = totalLen / EP_FLASH_SECT_LEN;
	fBuf = (uint8_t*) calloc(nSect, EP_FLASH_PKT_LEN);
	if (!fBuf) {
		perror("Allocating RAM for firmware");
			return -1;
	}

	// Open and read file
	if (!(fi = fopen(file, "r"))) {
		perror(file);
		return -1;
	}
	for (pos = 0, seq = 0, readed = 0; readed < fLen; seq++,
			readed += EP_FLASH_SECT_LEN) {
		// Fill header (length, seq, 0, 0) for this packet
		*((uint32_t*)(fBuf + pos)) = EP_FLASH_SECT_LEN;
		pos += sizeof(uint32_t);
		*((uint32_t*)(fBuf + pos)) = seq;
		pos += 3 * sizeof(uint32_t);
		// Read a sector
		toRead = MIN(EP_FLASH_SECT_LEN, fLen - readed);
		if (fread(&fBuf[pos], 1, toRead, fi) != toRead) {
			PrintErr("Reading firmware pos 0x%X:0x%X: ", readed, toRead);
			perror(NULL);
			fclose(fi);
			retVal = -1;
			goto blob_flash_free; 
		}
		pos += toRead;
	}
	fclose(fi);
	// 0xFF pad the buffer
	memset(fBuf + pos, 0xFF, nSect * EP_FLASH_PKT_LEN - pos);

	// Check if this is the first blob and correct flash parameters
	// if affirmative
	blobHdr = (EpBlobHdr*)(fBuf + EP_FLASH_PKT_HEAD_LEN);
	// TODO: Might be interesting allowing the user to configure these params
	if (0xE9 == blobHdr->magic && 0 == addr) {
		blobHdr->spiIf = EP_SPI_IF_QIO;
		blobHdr->flashParam = EP_FLASH_PARAM(EP_SPI_LEN_4M,
				EP_SPI_SPEED_40M);
	}
	
	// Enter bootloader
	EpReset();
	DelayMs(50);
	EpBootloader();
	DelayMs(5);
	EpRun();
	DelayMs(50);

	// Erase flash and prepare for data download
	printf("Erasing WiFi module, 0x%08X bytes at 0x%08X... ",
			(unsigned int) totalLen, addr); fflush(stdout);
	// Sync WiFi chip
	if (0 > EpProgSync()) {
		PrintErr("Error: Could not sync ESP8266!\n");
		retVal = -1;
		goto blob_flash_free;
	}

	if (0 > EpDownloadStart(totalLen, addr)) {
		PrintErr("Error, Could not erase flash!\n");
		retVal = -1;
		goto blob_flash_free;
	}
	printf("OK\n");

	// Flash blob, one sector each time.
	// TODO: WARNING, might need to unlock DIO
   	printf("Flashing WiFi firmware %s at 0x%06X...\n", file, addr);
	for (flashed = 0, i = 0; flashed < (nSect * EP_FLASH_PKT_LEN);
			flashed += EP_FLASH_PKT_LEN, i++) {
		sprintf(addrStr, "0x%08X", i * EP_FLASH_SECT_LEN);
		ProgBarDraw(i, nSect, f.cols, addrStr);
		// Calculate data checksum and send command with data and header
		csum = EpCsum(fBuf + flashed + EP_FLASH_PKT_HEAD_LEN,
				EP_FLASH_SECT_LEN);
		if (0 > EpSendCmd(EP_OP_FLASH_DOWNLOAD_DATA, fBuf + flashed,
					EP_FLASH_PKT_LEN, csum)) {
			PrintErr("Error flashing file %s at 0x%X\n", file,
					(uint32_t)flashed);
			retVal = -1;
			goto blob_flash_end;
		}
	}
	sprintf(addrStr, "0x%08X", i * EP_FLASH_SECT_LEN);
	ProgBarDraw(i, nSect, f.cols, addrStr);

blob_flash_end:
	// Send download finish command
	reboot = TRUE;
	EpSendCmd(EP_OP_FLASH_DOWNLOAD_FINISH, (uint8_t*)&reboot, sizeof(reboot),
			0);

blob_flash_free:
	// Free memory and return
	free(fBuf);
	return retVal;
}
