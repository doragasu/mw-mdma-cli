
//=============================================================================
// LIBS
//=============================================================================

#include "util.h"
#ifdef __OS_WIN
#include <windows.h>
#else
#include <sys/ioctl.h>
#endif

#include <getopt.h>
#include <errno.h>
#include "commands.h"
#include "progbar.h"
#include "esp-prog.h"

#define MAX_FILELEN		255
#define VERSION_MAJOR	0x00
#define VERSION_MINOR	0x02

/// Structure containing a memory image (file, address and length)
typedef struct {
	char *file;
	uint32_t addr;
	uint32_t len;
} MemImage;

//=============================================================================
// VARS
//=============================================================================
extern libusb_device_handle *megawifi_handle;
extern libusb_device *megawifi_dev;


const struct option opt[] = {
        {"flash",       required_argument,  NULL,   'f'},
        {"read",        required_argument,  NULL,   'r'},
        {"erase",       no_argument,        NULL,   'e'},
        {"sect-erase",  required_argument,  NULL,   's'},
        {"verify",      no_argument,        NULL,   'V'},
        {"flash-id",    no_argument,        NULL,   'i'},
		{"pushbutton",  no_argument,        NULL,   'p'},
        {"gpio-ctrl",   required_argument,  NULL,   'g'},
		{"wifi-flash",	required_argument,	NULL,	'w'},
        {"bootloader",  no_argument,        NULL,   'b'},
		{"dry-run",     no_argument,		NULL,   'd'},
        {"version",     no_argument,        NULL,   'R'},
        {"verbose",     no_argument,        NULL,   'v'},
        {"help",        no_argument,        NULL,   'h'},
        {NULL,          0,                 NULL,    0 }
};

const char *description[] = {
	"Flash rom file",
	"Read ROM/Flash to file",
	"Erase Flash",
	"Erase flash sector",
	"Verify flash after writing file",
	"Obtain flash chip identifiers",
	"Pushbutton status read (bit 1:event, bit0:pressed)",
	"Manual GPIO control (dangerous!)",
	"Upload firmware blob to WiFi module",
	"Switch to bootloader mode",
	"Dry run: don't actually do anything",
	"Show program version",
	"Show additional information",
	"Print help screen and exit"
};

/// 16-bit byte swap macro
#define ByteSwapWord(word)	do{(word) = ((word)>>8) | ((word)<<8);}while(0)

//=============================================================================
// FUNCTION DECLARATIONS
//=============================================================================

void PrintVersion(char *prgName) {
	printf("%s version %d.%d, Manveru & doragasu 2015.\n", prgName,
			VERSION_MAJOR, VERSION_MINOR);
}

void PrintHelp(char *prgName) {
	int i;

	PrintVersion(prgName);
	printf("Usage: %s [OPTIONS [OPTION_ARG]]\nSupported options:\n\n", prgName);
	for (i = 0; opt[i].name; i++) {
		printf(" -%c, --%s%s: %s.\n", opt[i].val, opt[i].name,
				opt[i].has_arg == required_argument?" <arg>":"",
				description[i]);
	}
	// Print additional info
	printf("For file arguments, it is possible to specify start address and "
		   "file length to read/write in words, with the following format:\n"
		   "    file_name:memory_address:file_length\n\n"
		   "Examples:\n"
		   " - Erase Flash and write entire ROM to cartridge: %s -ef rom_file\n"
		   " - Flash and verify 32 KiB to word address 0x700000: "
		   "%s -Vf rom_file:0x700000:16384\n"
		   " - Dump 1 MiB of the cartridge: %s -r rom_file::0x80000\n",
		   prgName, prgName, prgName);
		   
}

/// Receives a MemImage pointer with full info in file name (e.g.
/// m->file = "rom.bin:6000:1"). Removes from m->file information other
/// than the file name, and fills the remaining structure fields if info
/// is provided (e.g. info in previous example would cause m = {"rom.bin",
/// 0x6000, 1}).
int ParseMemArgument(MemImage *m) {
	int i;
	char *addr = NULL;
	char *len  = NULL;
	char *endPtr;

	// Set address and length to default values
	m->len = m->addr = 0;

	// First argument is name. Find where it ends
	for (i = 0; i < (MAX_FILELEN + 1) && m->file[i] != '\0' &&
			m->file[i] != ':'; i++);
	// Check if end reached without finding end of string
	if (i == MAX_FILELEN + 1) return 1;
	if (m->file[i] == '\0') return 0;
	
	// End of token marker, search address
	m->file[i++] = '\0';
	addr = m->file + i;
	for (; i < (MAX_FILELEN + 1) && m->file[i] != '\0' && m->file[i] != ':';
			i++);
	// Check if end reached without finding end of string
	if (i == MAX_FILELEN + 1) return 1;
	// If end of token marker, search length
	if (m->file[i] == ':') {
		m->file[i++] = '\0';
		len = m->file + i;
		// Verify there's an end of string
		for (; i < (MAX_FILELEN + 1) && m->file[i] != '\0'; i++);
		if (m->file[i] != '\0') return 1;
	}
	// Convert strings to numbers and return
	if (addr && *addr) m->addr = strtol(addr, &endPtr, 0);
	if (m->addr == 0 && addr == endPtr) return 2;
	if (len  && *len)  m->len  = strtol(len, &endPtr, 0);
	if (m->len  == 0 && len  == endPtr) return 3;

	return 0;
}

void PrintMemImage(MemImage *m) {
	printf("%s", m->file);
	if (m->addr) printf(" at address 0x%06X", m->addr);
	if (m->len ) printf(" (%d bytes)", m->len);
}

void PrintMemError(int code) {
	switch (code) {
		case 0: printf("Memory range OK.\n"); break;
		case 1: PrintErr("Invalid memory range string.\n"); break;
		case 2: PrintErr("Invalid memory address.\n"); break;
		case 3: PrintErr("Invalid memory length.\n"); break;
		default: PrintErr("Unknown memory specification error.\n");
	}
}

// Allocs a buffer, reads a file to the buffer, and flashes the file pointed 
// by the file argument. The buffer must be deallocated when not needed,
// using free() call.
// Note fWr.len is updated if not specified.
// Note buffer is byte swapped before returned.
u16 *AllocAndFlash(MemImage *fWr, int columns) {
    FILE *rom;
	u16 *writeBuf;
	uint32_t addr;
	int toWrite;
	uint32_t i;
	// Address string, e.g.: 0x123456
	char addrStr[9];

	if (!(rom = fopen(fWr->file, "rb"))) {
		perror(fWr->file);
		return NULL;
	}

	// Obtain length if not specified
	if (!fWr->len) {

	    fseek(rom, 0, SEEK_END);
	    fWr->len = ftell(rom)>>1;
	    fseek(rom, 0, SEEK_SET);
	}

    writeBuf = malloc(fWr->len<<1);
	if (!writeBuf) {
		perror("Allocating write buffer RAM");
		fclose(rom);
		return NULL;
	}
    fread(writeBuf, fWr->len<<1, 1, rom);
   	// Do byte swaps
   	for (i = 0; i < fWr->len; i++) ByteSwapWord(writeBuf[i]);

   	printf("Flashing ROM %s starting at 0x%06X...\n", fWr->file, fWr->addr);

	for (i = 0, addr = fWr->addr; i < fWr->len;) {
		toWrite = MIN(65536>>1, fWr->len - i);
		if (MDMA_write(toWrite, addr, writeBuf + i)) {
			free(writeBuf);
			fclose(rom);
			PrintErr("Couldn't write to cart!\n");
			return NULL;
		}
		// Update vars and draw progress bar
		i += toWrite;
		addr += toWrite;
   	    sprintf(addrStr, "0x%06X", addr);
   	    ProgBarDraw(i, fWr->len, columns, addrStr);
	}
   	putchar('\n');
	fclose(rom);
	return writeBuf;
}

// Allocs a buffer and reads from cart. Does NOT save the buffer to a file.
// Buffer must be deallocated using free() when not needed anymore.
u16 *AllocAndRead(MemImage *fRd, int columns) {
	u16 *readBuf;
	int toRead;
	uint32_t addr;
	uint32_t i;
	// Address string, e.g.: 0x123456
	char addrStr[9];

	readBuf = malloc(fRd->len<<1);
	if (!readBuf) {
		perror("Allocating read buffer RAM");
		return NULL;
	}
	printf("Reading cart starting at 0x%06X...\n", fRd->addr);

	fflush(stdout);
	for (i = 0, addr = fRd->addr; i < fRd->len;) {
		toRead = MIN(65536>>1, fRd->len - i);
		if (MDMA_read(toRead, addr, readBuf + i)) {
			free(readBuf);
			PrintErr("Couldn't read from cart!\n");
			return NULL;
		}
		fflush(stdout);
		// Update vars and draw progress bar
		i += toRead;
		addr += toRead;
   	    sprintf(addrStr, "0x%06X", addr);
   	    ProgBarDraw(i, fRd->len, columns, addrStr);
	}
	putchar('\n');
	return readBuf;
}

//-----------------------------------------------------------------------------
// MAIN
//-----------------------------------------------------------------------------
int main( int argc, char **argv )
{
	/// Command-line flags
	Flags f;
	/// Sector erase address. Set to UINT32_MAX for none
	int sect_erase = UINT32_MAX;
	/// Manual GPIO control
	// TODO: Replace with suitable structure
	int gpioCtl = FALSE;
	/// Rom file to write to flash
	MemImage fWr = {NULL, 0, 0};
	/// Rom file to read from flash (default read length: 4 MiB)
	MemImage fRd = {NULL, 0, 4*1024*1024};
	/// Binary blob to flash to the WiFi module
	MemImage fWf = {NULL, 0, 0};
	/// Error code for function calls
	int errCode;
	/// Buffer for writing data to cart
    u16 *write_buffer = NULL;
	/// Buffer for reading cart data
	u16 *read_buffer = NULL;

	// Just for loop iteration
	int i;

	// Set all flags to FALSE
	f.all = 0;
    // Reads console arguments
    if( argc > 1 )
    {
        /// Option index, for command line options parsing
        int opIdx = 0;
        /// Character returned by getopt_long()
        int c;

        while ((c = getopt_long(argc, argv, "f:r:es:Vipg:w:bdRvh", opt, &opIdx)) != -1)
        {
			// Parse command-line options
            switch (c)
            {
                case 'f': // Write flash
				fWr.file = optarg;
				if ((errCode = ParseMemArgument(&fWr))) {
					PrintErr("Error: On Flash file argument: ");
					PrintMemError(errCode);
					return 1;
				}
                break;

                case 'r': // Read flash
				fRd.file = optarg;
				if ((errCode = ParseMemArgument(&fRd))) {
					PrintErr("Error: On ROM/Flash read argument: ");
					PrintMemError(errCode);
					return 1;
				}
                break;

                case 'e': // Erase entire flash
                {
					f.erase = TRUE;
                }
                break;

				case 's': // Erase sector
                sect_erase = strtol( optarg, NULL, 16 );
				break;

                case 'V': // Verify flash write
				f.verify = TRUE;
                break;

                case 'i': // Flash id
				f.flashId = TRUE;
                break;

                case 'p': // Read pushbutton
				f.pushbutton = TRUE;
                break;

                case 'g': // GPIO control
				gpioCtl = TRUE;
                break;

				case 'w': // Flash WiFi firmware
				fWf.file = optarg;
				if ((errCode = ParseMemArgument(&fWf))) {
					PrintErr("Error: On WiFi firmware read argument. ");
					PrintMemError(errCode);
					return 1;
				}
				break;

                case 'b': // Bootloader mode
					f.boot = TRUE;
				break;

				case 'd': // Dry run
					f.dry = TRUE;
				break;

                case 'R': // Version
					PrintVersion(argv[0]);
                return 0;

                case 'v': // Verbose
					f.verbose = TRUE;
                break;

                case 'h': // Help
					PrintHelp(argv[0]);
                return 0;

                case '?':       // Unknown switch
					putchar('\n');
                	PrintHelp(argv[0]);
                return 1;
            }
        }
    }
    else
    {
		printf("Nothing to do!\n");
		PrintHelp(argv[0]);
		return 0;
}

	if (optind < argc) {
		PrintErr("Unsupported parameter:");
		for (i = optind; i < argc; i++) PrintErr(" %s", argv[i]);
		PrintErr("\n\n");
		PrintHelp(argv[0]);
		return -1;
	}

	if (f.verbose) {
		printf("\nThe following actions will%s be performed (in order):\n",
				f.dry?" NOT":"");
		printf("==================================================%s\n\n",
				f.dry?"====":"");
		if (f.flashId) printf(" - Show Flash chip identification.\n");
		if (f.erase) printf(" - Erase Flash.\n");
		else if (sect_erase != UINT32_MAX) 
			printf(" - Erase sector at 0x%X.\n", sect_erase);
		if (fWr.file) {
		   printf(" - Flash %s", f.verify?"and verify ":"");
		   PrintMemImage(&fWr); putchar('\n');
		}
		if (fRd.file) {
			printf(" - Read ROM/Flash to ");
			PrintMemImage(&fRd); putchar('\n');
		}
		if (f.pushbutton) {
			printf(" - Read pushbutton.\n");
		}
		if (gpioCtl) {
			printf(" - GPIO control (TODO).\n");
		}
		if (fWf.file) {
		   printf(" - Upload WiFi firmware ");
		   PrintMemImage(&fWf); putchar('\n');
		}
		if (f.boot) {
			printf(" - Enter bootloader\n");
		}
		printf("\n");
	}

	if (f.dry) return 0;

	// Detect number of columns (for progress bar drawing).
#ifdef __OS_WIN
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    f.cols = csbi.srWindow.Right - csbi.srWindow.Left;
#else
    struct winsize max;
    ioctl(0, TIOCGWINSZ , &max);
	f.cols = max.ws_col;

	// Also set transparent cursor
	printf("\e[?25l");
#endif
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
    else
        PrintVerb( "Completed: opened device %.4X : %.4X\n", MeGaWiFi_VID, MeGaWiFi_PID );

    megawifi_dev = libusb_get_device( megawifi_handle );


    // Set megawifi configuration
	r = libusb_set_configuration( megawifi_handle, MeGaWiFi_CONFIG );
	if( r < 0 ) {
        PrintErr( "Error: could not set configuration #%d\n", MeGaWiFi_CONFIG );
        PrintErr( "   Code: %s\n", libusb_error_name(r) );
        return -1;
	}
	else
        PrintVerb( "Completed: set configuration #%d\n", MeGaWiFi_CONFIG );


    // Claiming megawifi interface
	r = libusb_claim_interface( megawifi_handle, MeGaWiFi_INTERF );
    if( r != LIBUSB_SUCCESS )
    {
        PrintErr( "Error: could not claim interface #%d\n", MeGaWiFi_INTERF );
        PrintErr( "   Code: %s\n", libusb_error_name(r) );
        return -1;
    }
    else
        PrintVerb( "Completed: claim interface #%d\n", MeGaWiFi_INTERF );


	/****************** ↓↓↓↓↓↓ DO THE MAGIC HERE ↓↓↓↓↓↓ *******************/

	// Default exit status: OK
	errCode = 0;

	// GET IDs	
	if (f.flashId) {
		MDMA_manId_get();
		MDMA_devId_get();
	}
	// Erase
	// Support sector erase!
	if (f.erase) {
		printf("Erasing cart... ");
		fflush(stdout);
		// It looks like text doesn't appear until MDMA_cart_erase()
		// completes, so flush output to force it.
		if (MDMA_cart_erase() < 0) {
			printf("ERROR!\n");
			return 1;
		}
		else printf("OK!\n");
	} else if (sect_erase != UINT32_MAX) {
		printf("Erasing sector 0x%06X...\n", sect_erase);
		MDMA_sect_erase(sect_erase);
	}

	// Flash
	if (fWr.file) {
		write_buffer = AllocAndFlash(&fWr, f.cols);
		if (!write_buffer) {
			errCode = 1;
			goto dealloc_exit;
		}
	}

	if (fRd.file || f.verify) {
		// If verify is set, ignore addr and length set in command line.
		if (f.verify) {
			fRd.addr = fWr.addr;
			fRd.len  = fWr.len;
		}
		read_buffer = AllocAndRead(&fRd, f.cols);
		if (!read_buffer) {
			errCode = 1;
			goto dealloc_exit;
		}
		// Verify
		if (f.verify) {
			for (i = 0; i < fWr.len; i++) {
				if (write_buffer[i] != read_buffer[i]) {
					break;
				}
			}
			if (i == fWr.len)
				printf("Verify OK!\n");
			else {
				printf("Verify failed at addr 0x%07X!\n", i + fWr.addr);
				printf("Wrote: 0x%04X; Read: 0x%04X\n", write_buffer[i],
						read_buffer[i]);
				// Set error, but we do not exit yet, because user might want
				// to write readed data to a file!
				errCode = 1;
			}
		}
		// Write file
		if (fRd.file) {
			// Do byte swaps
			for (i = 0; i < fRd.len; i++) ByteSwapWord(read_buffer[i]);
        	FILE *dump = fopen(fRd.file, "wb");
			if (!dump) {
				perror(fRd.file);
				errCode = 1;
				goto dealloc_exit;
			}
	        fwrite(read_buffer, fRd.len<<1, 1, dump);
	        fclose(dump);
			printf("Wrote file %s.\n", fRd.file);
		}
	}

	if (f.pushbutton) {
		u16 retVal;
		u8 butStat;
		retVal = MDMA_button_get(&butStat);
		if (retVal < 0) {
			errCode = 1;
		} else {
			PrintVerb("Button status: 0x%02X.\n", butStat);
			errCode = butStat;
		}
		goto dealloc_exit;
	}

	if (gpioCtl)
		printf("Manual GPIO control not supported. Ignoring argument!!!\n");

	// WiFi Firmware upload
	if (fWf.file) {
		// Currlently, length argument is not supported, always the comlete
		// file is flashed.
		if (fWf.len) {
			PrintErr("Warning: length parameter not supported for WiFi "
					"firmware files, ignoring.\n");
			errCode = 1;
		}
		else {
			if (0 > EpBlobFlash(fWf.file, fWf.addr, f)) {
				PrintErr("Error while uploading WiFi firmware!\n");
				errCode = 1;
			}
//			Test code, remove!
//			while(1) {
//				EpReset();
//				EpProgStart();
//				DelayMs(500);
//				EpRun();
//				DelayMs(500);
//				EpReset();
//				EpBootloader();
//				DelayMs(500);
//				EpRun();
//				DelayMs(500);
//			}
		}
	}

dealloc_exit:
	if (write_buffer) free(write_buffer);
	if (read_buffer)  free(read_buffer);

	// Bootloader command is not replied!
	if (f.boot) MDMA_bootloader();

#ifndef __OS_WIN
	// Restore cursor
	printf("\e[?25h");
#endif
    r = libusb_release_interface( megawifi_handle, 0 );

    libusb_close( megawifi_handle );

    libusb_exit( NULL );


    return errCode;
}

