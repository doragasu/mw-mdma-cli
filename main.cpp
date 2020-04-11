
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
#include "mdma.h"

#if (defined(__OS_WIN) && defined(QT_STATIC))
// Windows static builds need to import Windows Integration plugin
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

#ifdef QT
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include "flashdlg.h"
#endif


//=============================================================================
// VARS
//=============================================================================

static const struct option opt[] = {
        {"qt-gui",      no_argument,        NULL,   'Q'},
        {"flash",       required_argument,  NULL,   'f'},
        {"read",        required_argument,  NULL,   'r'},
        {"erase",       no_argument,        NULL,   'e'},
        {"sect-erase",  required_argument,  NULL,   's'},
		{"range-erase", required_argument,  NULL,   'A'},
		{"auto-erase",  no_argument,		NULL,   'a'},
        {"verify",      no_argument,        NULL,   'V'},
        {"flash-id",    no_argument,        NULL,   'i'},
		{"pushbutton",  no_argument,        NULL,   'p'},
        {"gpio-ctrl",   required_argument,  NULL,   'g'},
		{"wifi-flash",	required_argument,	NULL,	'w'},
		{"wifi-mode",	required_argument,	NULL,	'm'},
        {"bootloader",  no_argument,        NULL,   'b'},
		{"dry-run",     no_argument,		NULL,   'd'},
        {"version",     no_argument,        NULL,   'R'},
        {"verbose",     no_argument,        NULL,   'v'},
        {"help",        no_argument,        NULL,   'h'},
        {NULL,          0,                  NULL,    0 }
};

static const char * const description[] = {
	"Start QT GUI",
	"Flash rom file",
	"Read ROM/Flash to file",
	"Erase Flash",
	"Erase flash sector",
	"Erase flash memory range",
	"Auto-erase (use it with flash command)",
	"Verify flash after writing file",
	"Obtain flash chip identifiers",
	"Pushbutton status read (bit 1:event, bit0:pressed)",
	"Manual GPIO control (dangerous!)",
	"Upload firmware blob to WiFi module",
	"Set WiFi module flash chip mode (qio, qout, dio, dout)",
	"Switch to bootloader mode",
	"Dry run: don't actually do anything",
	"Show program version",
	"Show additional information",
	"Print help screen and exit"
};

static const char * const esp_flash_mode_str[] = {
	"qio", "qout", "dio", "dout", NULL
};

//=============================================================================
// FUNCTION DECLARATIONS
//=============================================================================

static int str_index(const char *str_in, const char * const str_table[])
{
	int i = 0;

	while (str_table[i]) {
		if (!strcmp(str_in, str_table[i])) {
			return i;
		}
		i++;
	}

	return -1;
}

static void PrintVersion(const char *prgName) {
	printf("%s version %d.%d, Manveru & doragasu 2015-2017.\n", prgName,
			VERSION_MAJOR, VERSION_MINOR);
}

static void PrintHelp(char *prgName) {
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

//-----------------------------------------------------------------------------
// MAIN
//-----------------------------------------------------------------------------
int main( int argc, char **argv )
{
	/// Command-line flags
	Flags f;
	/// Sector erase address. Set to UINT32_MAX for none
	uint32_t sect_erase = UINT32_MAX;
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
	/// Address for memory erase operations
	uint32_t eraseAddr = 0;
	/// Length for memory erase operations
	uint32_t eraseLen = 0;
	// Manufacturer and device ids
	uint16_t ids[3];
	// Use QT GUI flag
	bool useQt = false;

	// Just for loop iteration
	int i;
	int aux;


	// Set default flag values
	f.all = 0;
	f.flash_mode = ESP_FLASH_UNCHANGED;

    // Reads console arguments
    if( argc > 1 )
    {
        /// Option index, for command line options parsing
        int opIdx = 0;
        /// Character returned by getopt_long()
        int c;

        while ((c = getopt_long(argc, argv, "Qf:r:es:A:aVipg:w:m:bdRvh", opt, &opIdx)) != -1)
        {
			// Parse command-line options
            switch (c)
            {
				case 'Q': // Use QT GUI
					useQt = true;
					break;

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
					f.erase = TRUE;
	                break;

				case 's': // Erase sector
	                sect_erase = strtol( optarg, NULL, 16 );
					break;

				case 'A': // Erase range
					if ((errCode = ParseMemRange(optarg, &eraseAddr, &eraseLen)) ||
							(0 == eraseLen)) {
						PrintErr("Error: Invalid Flash erase range argument: %s\n", optarg);
						return 1;
					}
					break;

				case 'a': // Auto erase
					f.auto_erase = TRUE;
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

				case 'm':
				aux = str_index(optarg, esp_flash_mode_str);
				if (aux < 0) {
					PrintErr("Invalid flash mode %s\n", optarg);
					return 1;
				}
				f.flash_mode = (enum esp_flash_mode)aux;
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

	// Try launching QT GUI if requested
	if (useQt) {
#ifdef QT
		QApplication app (argc, argv);

		// Try initialising USB device
		if (UsbInit() < 0) {
			QMessageBox::critical(NULL, "MDMA ERROR",
					"Could not find MDMA programmer!\n"
					"Please make sure MDMA programmer is\n"
					"plugged and drivers/permissions are OK.");
			return -1;
		}
	
		FlashDialog dlg;
		dlg.show();
		return app.exec();
#else
		PrintErr("Requested QT GUI, but MDMA has not been compiled with QT!\n");
		return -1;
#endif
	}

	if (optind < argc) {
		PrintErr("Unsupported parameter:");
		for (i = optind; i < argc; i++) PrintErr(" %s", argv[i]);
		PrintErr("\n\n");
		PrintHelp(argv[0]);
		return -1;
	}

	// Sanity checks
	if (f.auto_erase && !fWr.file) {
		PrintErr("Cannot auto-erase without writing to flash!\n");
		return -1;
	}
	if (f.auto_erase && (sect_erase != UINT32_MAX)) {
		PrintErr("Auto-erase and sector erase requested, aborting!\n");
		return -1;
	}
	if (f.auto_erase && f.erase) {
		PrintErr("Auto-erase and full erase requested, aborting!\n");
		return -1;
	}
	if (f.auto_erase && eraseLen) {
		PrintErr("Auto-erase and range erase requested, aborting!\n");
		return -1;
	}
	if ((sect_erase != UINT32_MAX) && eraseLen) {
		PrintErr("Sector erase and range erase requested, aborting!\n");
		return -1;
	}
	if ((sect_erase != UINT32_MAX) && f.erase) {
		PrintErr("Sector erase and full erase requested, aborting!\n");
		return -1;
	}
	if (eraseLen && f.erase) {
		PrintErr("Full erase and range erase requested, aborting!\n");
		return -1;
	}


	if (f.verbose) {
		printf("\nThe following actions will%s be performed (in order):\n",
				f.dry?" NOT":"");
		printf("==================================================%s\n\n",
				f.dry?"====":"");
		if (f.flashId) printf(" - Show Flash chip identification.\n");
		if (f.erase) printf(" - Erase Flash.\n");
		else if(f.auto_erase) printf(" - Auto-erase flash.\n");
		else if (eraseLen) {
			printf(" - Erase range 0x%X:%X.\n", eraseAddr, eraseLen);
		} else if (sect_erase != UINT32_MAX)
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
		   PrintMemImage(&fWf);
		   if (f.flash_mode < ESP_FLASH_UNCHANGED) {
			   printf(", mode: %s", esp_flash_mode_str[f.flash_mode]);
		   }
		   putchar('\n');
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

	if (UsbInit() < 0) PrintErr("Could not open MDMA programmer!\n");

	/****************** ↓↓↓↓↓↓ DO THE MAGIC HERE ↓↓↓↓↓↓ *******************/

	// Default exit status: OK
	errCode = 0;

	// GET IDs	
	if (f.flashId) {
		MDMA_manId_get(ids);
		printf("Manufacturer ID: 0x%04X\n", ids[0]);
		MDMA_devId_get(ids);
		printf("Device IDs: 0x%04X:%04X:%04X\n", ids[0], ids[1], ids[2]);
	}
	// Erase
	if (f.erase) {
		printf("Erasing cart... ");
		fflush(stdout);
		// It looks like text doesn't appear until MDMA_cart_erase()
		// completes, so flush output to force it.
		if (MDMA_cart_erase()) {
			printf("ERROR!\n");
			return 1;
		}
		else printf("OK!\n");
	} else if (sect_erase != UINT32_MAX) {
		printf("Erasing sector 0x%06X...\n", sect_erase);
		MDMA_sect_erase(sect_erase);
	} else if (eraseLen) {
		printf("Erasing range 0x%X:%X...\n", eraseAddr, eraseLen);
		MDMA_range_erase(eraseAddr, eraseLen);
	}

	// Flash
	if (fWr.file) {
		write_buffer = AllocAndFlash(&fWr, f.auto_erase, f.cols);
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
			for (i = 0; i < (int)fWr.len; i++) {
				if (write_buffer[i] != read_buffer[i]) {
					break;
				}
			}
			if (i == (int)fWr.len)
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
		   	for (i = 0; i < (int)fRd.len; i++) ByteSwapWord(read_buffer[i]);
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
		if (retVal) {
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
	UsbClose();
    return errCode;
}

