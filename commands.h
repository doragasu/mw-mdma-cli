#ifndef _COMMANDS_H_
#define _COMMANDS_H_


//=============================================================================
// LIBS
//=============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libusb-1.0/libusb.h>

#include "util.h"


//=============================================================================
// CONSTANTS
//=============================================================================
// Supported MDMAP commands
#define MDMA_OK             0 // Used to report OK status during command replies
#define MDMA_MANID_GET      1 // Flash chip manufacturer ID request
#define MDMA_DEVID_GET      2 // Flash chip device ID request
#define MDMA_READ           3 // Flash read request
#define MDMA_CART_ERASE     4 // Entire flash chip erase request
#define MDMA_SECT_ERASE     5 // Flash chip sector erase request
#define MDMA_WRITE          6 // Flash chip program request
#define MDMA_MAN_CTRL       7 // Manual GPIO control request (dangerous!, TBD)
#define MDMA_BOOTLOADER     8 // Enters DFU bootloader mode, to update MDMA firmware
#define MDMA_BUTTON_GET		9 // Gets pushbutton status.
#define MDMA_WIFI_CMD	   10 // Command forwarded to the WiFi chip.
#define MDMA_WIFI_CMD_LONG 11 // Long command forwarded to the WiFi chip.
#define MDMA_WIFI_CTRL     12 // WiFi chip control action (using GPIO pins).
#define MDMA_RANGE_ERASE   13 // Erase a memory range of the flash chip
#define MDMA_ERR          255 // Used to report ERROR status during command replies

typedef enum {
	MDMA_WIFI_CTRL_RST = 0,	// Hold chip in reset state.
	MDMA_WIFI_CTRL_RUN,		// Reset the chip.
	MDMA_WIFI_CTRL_BLOAD,	// Enter bootloader mode.
	MDMA_WIFI_CTRL_APP,		// Start application.
	MDMA_WIFI_CTRL_SYNC 	// Perform a sync attemp.
} MdmaWifiCtrlCode;


#define MeGaWiFi_VID                0x03EB
#define MeGaWiFi_PID                0x206C

#define MeGaWiFi_BOOTLOADER_VID     0x03EB
#define MeGaWiFi_BOOTLOADER_PID     0x2FF9

#define MeGaWiFi_ENDPOINT_IN        0x83
#define MeGaWiFi_ENDPOINT_OUT       0x04

#define MeGaWiFi_CONFIG             1
#define MeGaWiFi_INTERF             0

#define PAYLOAD_OFFSET			6

#define WIFI_PAYLOAD_OFFSET		4

#define ENDPOINT_LENGTH			64

#define MAX_PAYLOAD_BYTES		(ENDPOINT_LENGTH - PAYLOAD_OFFSET)

#define MAX_WIFI_PAYLOAD_BYTES 	(ENDPOINT_LENGTH - WIFI_PAYLOAD_OFFSET)

// Can be up to 512 bytes, but it looks like 384 is the
// optimum value to maximize speed
#define MAX_USB_TRANSFER_LEN	384


typedef union
{
    u8 bytes[MAX_PAYLOAD_BYTES + 6];

    struct
    {
        u8 cmd; // Supported MDMAP commands
        u8 len[2]; // 1 to 1024
        u8 addr[3];
        u8 data[MAX_PAYLOAD_BYTES];

    } frame;

	struct {
		u8 cmd;
		u8 addr[3];
		u8 dwlen[4];
	} erase;

	struct {
		u8 cmd;
		u8 len[2];
		u8 pad;
		u8 data[MAX_WIFI_PAYLOAD_BYTES];
	} WiFiFrame;

} Command;

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================
u16 MDMA_manId_get();

u16 MDMA_devId_get();

u16 MDMA_read( u16 wLen, int addr, u16 * data );

u16 MDMA_cart_erase();

u16 MDMA_sect_erase( int addr );

u16 MDMA_range_erase(uint32_t addr, uint32_t length);

u16 MDMA_write( u16 wLen, int addr, u16 * data );

u16 MDMA_bootloader();

u16 MDMA_button_get(uint8_t *button_status);

int MDMA_WiFiCmd(uint8_t *payload, uint8_t len, uint8_t *reply);

int MDMA_WiFiCmdLong(uint8_t *payload, uint16_t len, uint8_t *reply);

int MDMA_WiFiCtrl(MdmaWifiCtrlCode code);

#ifdef __cplusplus
}
#endif

#endif // _COMMANDS_H_
