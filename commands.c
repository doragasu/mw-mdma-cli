
//=============================================================================
// LIBS
//=============================================================================
#include "commands.h"
#include "util.h"


//=============================================================================
// CONSTANTS
//=============================================================================

// Commands byte size
#define COMMAND_FRAME_BYTES     ENDPOINT_LENGTH

// Default time to make a bulk transfer
#define REGULAR_TIMEOUT         3000
#define CART_ERASE_TIMEOUT      70000
//#define RETRIES         3

//=============================================================================
// VARS
//=============================================================================
static libusb_device_handle *megawifi_handle; // The megawifi device handle.
static libusb_device *megawifi_dev;


//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================
int megawifi_bulk_send_command( s8 * cmd_name, Command * command );
int megawifi_bulk_get_reply_data( Command * command, u16 *buffer, u16 length, int timeout );

//=============================================================================
// FUNCTION DECLARATIONS
//=============================================================================

/// USB initialization
int UsbInit(void) {
    // Init libusb
    int r = libusb_init(NULL);
	if (r < 0) {
        PrintErr( "Error: could not init libusb\n" );
        PrintErr( "   Code: %s\n", libusb_error_name(r) );
        return -1;
    }

	// Uncomment this to flood the screen with libusb debug information
	//libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);


    // Detecting megawifi device
	megawifi_handle = libusb_open_device_with_vid_pid( NULL, MeGaWiFi_VID, MeGaWiFi_PID );

	if( megawifi_handle == NULL ) {
		PrintErr( "Error: could not open device %.4X : %.4X\n", MeGaWiFi_VID, MeGaWiFi_PID );
		return -1;
	}

    megawifi_dev = libusb_get_device( megawifi_handle );


    // Set megawifi configuration
	r = libusb_set_configuration( megawifi_handle, MeGaWiFi_CONFIG );
	if( r < 0 ) {
        PrintErr( "Error: could not set configuration #%d\n", MeGaWiFi_CONFIG );
        PrintErr( "   Code: %s\n", libusb_error_name(r) );
        return -1;
	}

    // Claiming megawifi interface
	r = libusb_claim_interface( megawifi_handle, MeGaWiFi_INTERF );
    if( r != LIBUSB_SUCCESS )
    {
        PrintErr( "Error: could not claim interface #%d\n", MeGaWiFi_INTERF );
        PrintErr( "   Code: %s\n", libusb_error_name(r) );
        return -1;
    }
	return 0;
}

/// Ends USB session with device
void UsbClose(void) {
    libusb_release_interface( megawifi_handle, 0 );

    libusb_close( megawifi_handle );

    libusb_exit( NULL );
}



//-----------------------------------------------------------------------------
// MDMA_MANID_GET
//-----------------------------------------------------------------------------
u16 MDMA_manId_get(uint16_t *manId)
{
    Command command_out = { { MDMA_MANID_GET } };
    Command command_in; // CMD byte + MANID word.
    int r;

    r = megawifi_bulk_send_command( "MANID_GET", &command_out );
    if( r < 0 ) return -1;

    r = megawifi_bulk_get_reply_data( &command_in, NULL, 0, REGULAR_TIMEOUT );
    if( r < 0 ) return -1;


    // Checks read info
    if( command_in.frame.cmd == MDMA_OK ) {
        u16 * code = ( u16 * ) &command_in.bytes[1];
		*manId = *code;
    }
    else
        printf( "Command field byte = 0x%.2X (MDMA_ERR) \n", command_in.frame.cmd );


    return 0;
}

//-----------------------------------------------------------------------------
// MDMA_DEVID_GET
//-----------------------------------------------------------------------------
u16 MDMA_devId_get(uint16_t devId[3])
{
    Command command_out = { { MDMA_DEVID_GET } };
    Command command_in; // CMD byte + DEVID 3 words.

    int r;

    r = megawifi_bulk_send_command( "DEVID_GET", &command_out );
    if( r < 0 ) return -1;

    r = megawifi_bulk_get_reply_data( &command_in, NULL, 0, REGULAR_TIMEOUT );
    if( r < 0 ) return -1;


    // Checks read info
    if( command_in.frame.cmd == MDMA_OK ) {
        u16 * code = ( u16 * ) &command_in.bytes[1];
		devId[0] = code[0];
		devId[1] = code[1];
		devId[2] = code[2];
    }
    else
        printf( "Command field byte = 0x%.2X (MDMA_ERR) \n", command_in.frame.cmd );


    return 0;
}

//-----------------------------------------------------------------------------
// MDMA_READ
//-----------------------------------------------------------------------------
u16 MDMA_read( u16 wLen, int addr, u16 * data )
{
    Command command_out = { { MDMA_READ } };
    Command command_in;

    int r;

	// Write payload length
	command_out.frame.len[0] = wLen & 0xFF;
	command_out.frame.len[1] = wLen>>8;

	// Write address
	command_out.frame.addr[0] = addr & 0xFF;
	command_out.frame.addr[1] = (addr>>8) & 0xFF;
	command_out.frame.addr[2] = (addr>>16) & 0xFF;

    r = megawifi_bulk_send_command( "READ", &command_out );
    if( r < 0 ) return -1;

    r = megawifi_bulk_get_reply_data(&command_in, data, wLen, REGULAR_TIMEOUT);
    if( r < 0 ) return -1;

    // Checks read info
    if( command_in.frame.cmd != MDMA_OK ) {
        printf( "Command field byte = 0x%.2X (MDMA_ERR) \n", command_in.frame.cmd );
        printf( "Error: could not read %d word(s) from address 0x%.8X \n", wLen, addr );

        return -1;
    }
    return 0;
}

//-----------------------------------------------------------------------------
// MDMA_CART_ERASE
//-----------------------------------------------------------------------------
u16 MDMA_cart_erase()
{
    Command command_out = { { MDMA_CART_ERASE } };
    Command command_in;
    int r;

    r = megawifi_bulk_send_command( "CART_ERASE", &command_out );
    if( r < 0 ) return -1;

    //printf( "Erasing flash chip...\n" );

    r = megawifi_bulk_get_reply_data( &command_in, NULL, 0, CART_ERASE_TIMEOUT );
    if( r < 0 ) return -1;

    // Checks read info
    if( command_in.frame.cmd == MDMA_OK ) {
    }
    else {
        printf( "\nCommand field byte = 0x%.2X (MDMA_ERR) \n", command_in.frame.cmd );
        printf( "Error: flash chip was not erased \n" );
    }


    return 0;
}

//-----------------------------------------------------------------------------
// MDMA_RANGE_ERASE
//-----------------------------------------------------------------------------
u16 MDMA_range_erase(uint32_t addr, uint32_t length) {
	Command command_out = {{MDMA_RANGE_ERASE}};
	Command command_in;
	int r;

	command_out.erase.addr[0] = addr & 0xFF;
	command_out.erase.addr[1] = (addr>>8)  & 0xFF;
	command_out.erase.addr[2] = (addr>>16) & 0xFF;
	command_out.erase.dwlen[0] = length & 0xFF;
	command_out.erase.dwlen[1] = (length>>8)  & 0xFF;
	command_out.erase.dwlen[2] = (length>>16) & 0xFF;
	command_out.erase.dwlen[3] = (length>>24) & 0xFF;

	r = megawifi_bulk_send_command( "RANGE ERASE", &command_out );
    if( r < 0 ) return -1;

    r = megawifi_bulk_get_reply_data( &command_in, NULL, 0, CART_ERASE_TIMEOUT );
    if( r < 0 ) return -1;


    // Checks read info
    if( command_in.frame.cmd != MDMA_OK ) {
        printf( "Command field byte = 0x%.2X (MDMA_ERR) \n", command_in.frame.cmd );
        printf( "Error: could not erase flash at 0x%X:%X \n", addr, length );
    }



	return 0;
}

//-----------------------------------------------------------------------------
// MDMA_SECT_ERASE
//-----------------------------------------------------------------------------
u16 MDMA_sect_erase( int addr )
{
    Command command_out = { { MDMA_SECT_ERASE } };
    Command command_in;
    int r;

    int * addr_pointer = ( int * ) &command_out.bytes[1];
    *addr_pointer = addr;

    r = megawifi_bulk_send_command( "SECT_ERASE", &command_out );
    if( r < 0 ) return -1;

    r = megawifi_bulk_get_reply_data( &command_in, NULL, 0, REGULAR_TIMEOUT );
    if( r < 0 ) return -1;


    // Checks read info
    if( command_in.frame.cmd != MDMA_OK ) {
        printf( "Command field byte = 0x%.2X (MDMA_ERR) \n", command_in.frame.cmd );
        printf( "Error: could not erase sector at 0x%.8X \n", addr );
    }


    return 0;
}

//-----------------------------------------------------------------------------
// MDMA_WRITE
//-----------------------------------------------------------------------------
u16 MDMA_write( u16 wLen, int addr, u16 * data )
{
    Command command_out = { { MDMA_WRITE } };
    Command command_in;

    int r;
	int size;

	// Write payload length
	command_out.frame.len[0] = wLen & 0xFF;
	command_out.frame.len[1] = wLen>>8;

	// Write address
	command_out.frame.addr[0] = addr & 0xFF;
	command_out.frame.addr[1] = (addr>>8) & 0xFF;
	command_out.frame.addr[2] = (addr>>16) & 0xFF;

    r = megawifi_bulk_send_command( "WRITE", &command_out );
    if( r < 0 ) return -1;

    r = megawifi_bulk_get_reply_data( &command_in, NULL, 0, REGULAR_TIMEOUT );
    if( r < 0 ) return -1;

    if( command_in.frame.cmd == MDMA_OK ) {
		// Send big data payload
		r = libusb_bulk_transfer(megawifi_handle, MeGaWiFi_ENDPOINT_OUT,
				((unsigned char*)data), wLen<<1, &size, REGULAR_TIMEOUT);

		if (r != LIBUSB_SUCCESS && size != (wLen<<1)) {
			PrintErr("Error: couldn't write payload!\n");
			PrintErr("   Code: %s\n", libusb_error_name(r) );
		}
		
    }
    else {
        printf( "Command field byte = 0x%.2X (MDMA_ERR) \n", command_in.frame.cmd );
        printf( "Error: could not send %d word(s) at address 0x%.8X \n", wLen, addr );

        return -1;
    }

    return 0;
}

//-----------------------------------------------------------------------------
// MDMA_BOOTLOADER
//-----------------------------------------------------------------------------
u16 MDMA_bootloader()
{
    Command command_out = { { MDMA_BOOTLOADER } };
    int r;

    r = megawifi_bulk_send_command( "BOOTLOADER", &command_out );
    if( r < 0 ) return -1;

    return 0;
}

//-----------------------------------------------------------------------------
// MDMA_BUTTON_GET
//-----------------------------------------------------------------------------
u16 MDMA_button_get(uint8_t *button_status)
{
    Command command_out = { { MDMA_BUTTON_GET } };
	Command command_in;
    int r;

    r = megawifi_bulk_send_command( "BUTTON_GET", &command_out );
    if( r < 0 ) return -1;

    r = megawifi_bulk_get_reply_data( &command_in, NULL, 0, REGULAR_TIMEOUT );
    if( r < 0 ) return -1;

	*button_status = command_in.frame.len[0];

    return 0;
}





//-----------------------------------------------------------------------------
// MEGAWIFI_BULK_SEND_COMMAND
//-----------------------------------------------------------------------------
int megawifi_bulk_send_command( s8 * cmd_name, Command * command )
{
    int ret;
    int size;

    ret = libusb_bulk_transfer( megawifi_handle, MeGaWiFi_ENDPOINT_OUT,
        command->bytes, COMMAND_FRAME_BYTES, &size, REGULAR_TIMEOUT );

    if( ret != LIBUSB_SUCCESS && size != COMMAND_FRAME_BYTES )
    {
		printf( "Error: bulk transfer can not send %s command \n",
            cmd_name );

		printf( "   Code: %s\n", libusb_error_name(ret) );

		return -1;
	}

    return 0;
}

//-----------------------------------------------------------------------------
// MEGAWIFI_BULK_GET_REPLY_DATA
//-----------------------------------------------------------------------------
int megawifi_bulk_get_reply_data( Command * command, u16 *buffer, u16 length, int timeout )
{
    int ret;
    int size;
	u16 recvd = 0;
	u16 step;

	// Receive the reply to the command
    ret = libusb_bulk_transfer( megawifi_handle,
        MeGaWiFi_ENDPOINT_IN, command->bytes, COMMAND_FRAME_BYTES, &size, timeout );

    if( ret != LIBUSB_SUCCESS && size != COMMAND_FRAME_BYTES ) {
		printf( "Error: bulk transfer reply failed \n" );
		printf( "   Code: %s\n", libusb_error_name(ret) );
		return -1;
	}

	if (buffer && length) {
		// Now receive the big data payload
		while (recvd < length) {
			step = MIN(MAX_USB_TRANSFER_LEN, (length - recvd)<<1);
			ret = libusb_bulk_transfer(megawifi_handle, MeGaWiFi_ENDPOINT_IN,
					(unsigned char*)(buffer+recvd), step, &size, timeout);
		
			if (ret != LIBUSB_SUCCESS && size != step) {
				PrintErr("Error: couldn't get read payload!\n");
				PrintErr("   Code: %s\n", libusb_error_name(ret) );
			}
			recvd += step>>1;
		}
	}

    return 0;
}

/// \note payload is padded to 32 bits, both for sending and receiving
int MDMA_WiFiCmd(uint8_t *payload, uint8_t len, uint8_t *reply) {
	int r;
	uint8_t i;
	int recvLen;

	if (len > MAX_WIFI_PAYLOAD_BYTES) return 0;

    Command command_out = { { MDMA_WIFI_CMD } };
	Command command_in;
	
	// Write payload length
	command_out.WiFiFrame.len[0] = len;
	command_out.WiFiFrame.len[1] = 0;

	// Write data
	for (i = 0; i < len; i++)
		command_out.WiFiFrame.data[i] = payload[i];

	// Send command and get response
	r = megawifi_bulk_send_command("WIFI_CMD", &command_out);
    if( r < 0 ) return -1;

    r = megawifi_bulk_get_reply_data( &command_in, NULL, 0, CART_ERASE_TIMEOUT );
    if( r < 0 ) return -1;

	recvLen = MIN(MAX_WIFI_PAYLOAD_BYTES, command_in.WiFiFrame.len[0]);
	for (i = 0; i < recvLen; i++) reply[i] = command_in.WiFiFrame.data[i];

	return recvLen;
}

int MDMA_WiFiCmdLong(uint8_t *payload, uint16_t len, uint8_t *reply) {
	int r;
	uint8_t i;
	int recvLen, size;

    Command command_out = { { MDMA_WIFI_CMD_LONG } };
	Command command_in;
	
	// Write payload length
	command_out.WiFiFrame.len[0] = len & 0xFF;
	command_out.WiFiFrame.len[1] = len>>8;

	// Send command
	r = megawifi_bulk_send_command("WIFI_CMD_LONG", &command_out);
    if( r < 0 ) return -1;

	// Send big data chunck
	r = libusb_bulk_transfer(megawifi_handle, MeGaWiFi_ENDPOINT_OUT,
			payload, len, &size, REGULAR_TIMEOUT);

	if (r != LIBUSB_SUCCESS && size != len) {
		PrintErr("Error: couldn't write payload!\n");
		PrintErr("   Code: %s\n", libusb_error_name(r) );
	}
	
	// Get response
    r = megawifi_bulk_get_reply_data(&command_in, NULL, 0, REGULAR_TIMEOUT);
    if( r < 0 ) return -1;

	recvLen = MIN(MAX_WIFI_PAYLOAD_BYTES, command_in.WiFiFrame.len[0]);
	for (i = 0; i < recvLen; i++) reply[i] = command_in.WiFiFrame.data[i];

	return recvLen;
}

int MDMA_WiFiCtrl(MdmaWifiCtrlCode code) {
	int r;

    Command command_out = { { MDMA_WIFI_CTRL } };
	Command command_in;
	
	
	// Write control code
	command_out.bytes[1] = code;
	
	// Exception to the norm: if SYNC, write the number of retries
	if (code == MDMA_WIFI_CTRL_SYNC) command_out.bytes[2] = 250;

	// Send command
	r = megawifi_bulk_send_command("WIFI_CTRL", &command_out);
    if( r < 0 ) return -1;

	// Get response
    r = megawifi_bulk_get_reply_data(&command_in, NULL, 0, REGULAR_TIMEOUT);
    if( r < 0 ) return -1;

	// TODO: Check status code of returned data!

	return command_in.bytes[1];
}

