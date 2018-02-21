/******************************************************************************
 * MSP432 printf example project
 *
 * Description: An example project to illustrate printf usage on the MSP432
 * Launchpad.

 * Author: Gerard Sequeira, bluehash@43oh
 *******************************************************************************/

/* DriverLib Includes */
#include "driverlib.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "clock_driver.h"
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"
#include "spiDriver.h"

// Defines the size of the buffers that hold the path, or temporary data from
// the SD card.  There are two buffers allocated of this size.  The buffer size
// must be large enough to hold the longest expected full path name, including
// the file name, and a trailing null character.
#define PATH_BUF_SIZE           80

// A temporary data buffer used when manipulating file paths, or reading data
// from the SD card.
static char g_pcTmpBuf[PATH_BUF_SIZE];

// The following are data structures used by FatFs.
static FATFS g_sFatFs;
static FIL g_sFileObject;



// A structure that holds a mapping between an FRESULT numerical code, and a
// string representation.  FRESULT codes are returned from the FatFs FAT file
// system driver.
typedef struct {
	FRESULT iFResult;
	char *pcResultStr;
} tFResultString;

// A macro to make it easy to add result codes to the table.
#define FRESULT_ENTRY(f)        { (f), (#f) }

// A table that holds a mapping between the numerical FRESULT code and it's
// name as a string.  This is used for looking up error codes for printing to
// the console.
tFResultString g_psFResultStrings[] = {
FRESULT_ENTRY(FR_OK),
FRESULT_ENTRY(FR_DISK_ERR),
FRESULT_ENTRY(FR_INT_ERR),
FRESULT_ENTRY(FR_NOT_READY),
FRESULT_ENTRY(FR_NO_FILE),
FRESULT_ENTRY(FR_NO_PATH),
FRESULT_ENTRY(FR_INVALID_NAME),
FRESULT_ENTRY(FR_DENIED),
FRESULT_ENTRY(FR_EXIST),
FRESULT_ENTRY(FR_INVALID_OBJECT),
FRESULT_ENTRY(FR_WRITE_PROTECTED),
FRESULT_ENTRY(FR_INVALID_DRIVE),
FRESULT_ENTRY(FR_NOT_ENABLED),
FRESULT_ENTRY(FR_NO_FILESYSTEM),
FRESULT_ENTRY(FR_MKFS_ABORTED),
FRESULT_ENTRY(FR_TIMEOUT),
FRESULT_ENTRY(FR_LOCKED),
FRESULT_ENTRY(FR_NOT_ENOUGH_CORE),
FRESULT_ENTRY(FR_TOO_MANY_OPEN_FILES),
FRESULT_ENTRY(FR_INVALID_PARAMETER), };

// A macro that holds the number of result codes.
#define NUM_FRESULT_CODES       (sizeof(g_psFResultStrings) /                 \
                                 sizeof(tFResultString))


// This function returns a string representation of an error code that was
// returned from a function call to FatFs.  It can be used for printing human
// readable error messages.
const char *
StringFromFResult(FRESULT iFResult) {
	uint_fast8_t ui8Idx;

	// Enter a loop to search the error code table for a matching error code.
	for (ui8Idx = 0; ui8Idx < NUM_FRESULT_CODES; ui8Idx++) {
		// If a match is found, then return the string name of the error code.
		if (g_psFResultStrings[ui8Idx].iFResult == iFResult) {
			return (g_psFResultStrings[ui8Idx].pcResultStr);
		}
	}

	// At this point no matching code was found, so return a string indicating
	// an unknown error.
	return ("UNKNOWN ERROR CODE");
}

/* UART Configuration Parameter. These are the configuration parameters to
 * make the eUSCI A UART module to operate with a 115200 baud rate. These
 * values were calculated using the online calculator that TI provides
 * at:
 * http://processors.wiki.ti.com/index.php/
 *               USCI_UART_Baud_Rate_Gen_Mode_Selection
 */
const eUSCI_UART_Config uartConfig = {
        EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source 12MHz
		104,                                     // BRDIV = 104
		0,                                       // UCxBRF = 0
		1,                                       // UCxBRS = 1
		EUSCI_A_UART_NO_PARITY,                  // No Parity
		EUSCI_A_UART_LSB_FIRST,                  // MSB First
		EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
		EUSCI_A_UART_MODE,                       // UART mode
		EUSCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION  // Low Frequency Mode
		};

void SysTick_ISR(void) {
	// Call the FatFs tick timer.
	disk_timerproc();

	MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
}



int main(void) {

	FRESULT iFResult;
	UINT readBytes = 0;
	UINT writeBytes = 0;

	/* Halting WDT and disabling master interrupts */
	WDTCTL = WDTPW | WDTHOLD;                 // Stop WDT

	/* Initialize main clock to 3MHz */
	clockInit48MHzXTL();

	/* Selecting P1.0 as output (LED). */
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
	GPIO_PIN0, GPIO_PRIMARY_MODULE_FUNCTION);

	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

	/* Selecting P1.2 and P1.3 in UART mode. */
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
	GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

	// configure and enable uart
	MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);
	MAP_UART_enableModule(EUSCI_A0_BASE);

	MAP_Interrupt_enableMaster();

	// Configure SysTick for a 100Hz.  The FatFs driver wants 10 ms tick.
	MAP_SysTick_setPeriod(48000000 / 100);
	MAP_SysTick_enableModule();
	MAP_SysTick_enableInterrupt();

	spi_Open();



	// Print message to user.
    printf("\n\nSD Card Test Program\n\r");

    // Mount the file system, using logical disk 0.
    iFResult = f_mount(0, &g_sFatFs);
    if (iFResult != FR_OK)
    {
        printf("f_mount error: %s\n\r", StringFromFResult(iFResult));
    }

    // Open a new file in the fileobject
    iFResult = f_open(&g_sFileObject, "newFile1", FA_WRITE | FA_CREATE_ALWAYS);
    if (iFResult != FR_OK)
    {
        printf("f_open error: %s\n\r", StringFromFResult(iFResult));
    }

    // write a 16 byte string to the sd card
    iFResult = f_write(&g_sFileObject, "This is a test\r\n", 16, &writeBytes);
    if (iFResult != FR_OK)
    {
        printf("f_write error: %s\n\r", StringFromFResult(iFResult));
    }

    // print the number of characters written and close the file
    printf("%d characters written\n\r", writeBytes);
    iFResult = f_close(&g_sFileObject);
    if (iFResult != FR_OK)
    {
        printf("f_close error: %s\n\r", StringFromFResult(iFResult));
    }

    // open the file
    iFResult = f_open(&g_sFileObject, "newFile1", FA_READ);
    if (iFResult != FR_OK)
    {
        printf("f_open error: %s\n\r", StringFromFResult(iFResult));
    }

    // read a string from the file on the sd card
    iFResult = f_read(&g_sFileObject, g_pcTmpBuf, sizeof(g_pcTmpBuf) - 1, &readBytes);
    if (iFResult != FR_OK)
    {
        printf("f_read error: %s\n\r", StringFromFResult(iFResult));
    }

    // print the number of characters read and the line read
    printf("%d characters read\n\r",readBytes);
    printf("%s\n\r", g_pcTmpBuf);

    // close the file again
    iFResult = f_close(&g_sFileObject);
    if (iFResult != FR_OK)
    {
        printf("f_close error: %s\n\r", StringFromFResult(iFResult));
    }

}
