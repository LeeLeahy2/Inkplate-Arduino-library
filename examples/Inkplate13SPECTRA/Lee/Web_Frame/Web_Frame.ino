/**
 **************************************************
 * @file        Web_Frame.ino
 * @brief       Displays images from an SD card or the network.
 *
 * @details     Started from Inkplate13SPECTRA_Image_Frame_From_Web.ino.
 *              Loads a .CSV file listing the local file names and their
 *              URL to access on the network.  Loads the image from the
 *              SD card if possible and from the network if not.
 *
 * Requirements:
 * - Board:      Soldered Inkplate 13SPECTRA
 * - Hardware:   Inkplate 13SPECTRA, USB cable
 * - Extra:      WiFi Internet connection
 *
 * How to use:
 * 1) In Boards Manager -> Inkplate Boards, select "Soldered Inkplate 13SPECTRA"
 *    from Tools -> Board.
 * 2) Enter your WiFi credentials in the sketch.
 * 3) Upload the sketch to Inkplate 13SPECTRA.
 * 4) A random picture is downloaded and shown on the display, refreshed
 *    periodically.
 *
 * @note        Quick start guide:
 *              https://docs.soldered.com/inkplate/13spectra/quick-start-guide/
 * @note        Want to learn more about Inkplate? Visit
 *              https://docs.soldered.com/inkplate/
 * @note        Looking to get support? Write on our community forum:
 *              https://community.soldered.com/
 *
 * @author      Lee Leahy
 * @date        2026-10-16
 **************************************************/

// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#ifndef ARDUINO_INKPLATE13SPECTRA
#error "Wrong board selection for this example, please select Soldered Inkplate 6COLOR in the boards menu."
#endif

#include <arpa/inet.h>
#include <HTTPClient.h>
#include <netdb.h>
#include <Network.h>
#include <NetworkClientSecure.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiMulti.h>

#include "Inkplate.h"
#include "secrets.h"

//----------------------------------------
// Constants
//----------------------------------------

#define CHARGE_VOLTAGE                  3.9

#define HEADER_LINE_COUNT               5

#define MICROSECONDS_IN_A_MILLISECOND   1000ull
#define MILLISECONDS_IN_A_SECOND        1000ull
#define SECONDS_IN_A_MINUTE             60ull

#define MICROSECONDS_IN_A_SECOND    (MICROSECONDS_IN_A_MILLISECOND * MILLISECONDS_IN_A_SECOND)
#define MICROSECONDS_IN_A_MINUTE    (MICROSECONDS_IN_A_SECOND * SECONDS_IN_A_MINUTE)

#define NETWORK_TIMEOUT             ((uint32_t)(15 * MICROSECONDS_IN_A_MINUTE))
#define DATA_TIMEOUT                (15 * MILLISECONDS_IN_A_SECOND)

#define DISPLAY_WIDTH       1600
#define DISPLAY_HEIGHT      1200

//----------------------------------------
// Globals
//----------------------------------------

char batteryVoltageString[64];
Print * debugPrint;
Inkplate display;

volatile RTC_DATA_ATTR uint32_t imageNumber;    // Image number to display
volatile RTC_DATA_ATTR uint16_t displayMinutes; // Image display time in minutes
volatile RTC_DATA_ATTR uint16_t imagesBeforeCsvUpdate;  // Time between CSV file updates
                                                        // ~= displayMinutes * imagesBeforeCsvUpdate
volatile RTC_DATA_ATTR bool debuggingEnabled;   // Enable or disable debugging output
volatile RTC_DATA_ATTR bool displayFiles;       // Display files when true

//----------------------------------------
// Type declarations
//----------------------------------------

// Comma separated list information
typedef struct _CSV_ENTRY
{
    const char * _filePath;
    const char * _url;
} CSV_ENTRY;

// Context passed to processCsvLines routine
typedef struct _CSV_CONTEXT
{
    const CSV_ENTRY * _csvTable;
    uint32_t _lineCount;
} CSV_CONTEXT;

// Callback routine to process lines of the CSV file
typedef bool (* NETWORK_PROCESS_CSV_LINES)(const CSV_ENTRY * csvTable,
                                           uint32_t lineCount);

// File information
typedef struct _FILE_INFO
{
    struct _FILE_INFO * _next;
    const char * _filePath;
    size_t _fileBytes;
    bool _isDirectory;
} FILE_INFO;

// Callback routine to process files from directory listing
typedef bool (* SD_CARD_PROCESS_FILES)(const FILE_INFO ** fileTable,
                                       uint32_t fileCount,
                                       void * context);

//----------------------------------------
// Starting point for the application
//----------------------------------------
void setup()
{
    bool success;

    Serial.begin(115200);

    // Display a sign-on message
    Serial.println("Web_Frame application");

    // Set the default values of the parameters
    if (displayMinutes == 0)
        displayMinutes = 15;
    if (imagesBeforeCsvUpdate == 0)
        imagesBeforeCsvUpdate = 4;

    // Select the debug serial output
    debugPrint = debuggingEnabled ? &Serial : nullptr;

    // Initialize the display
    display.begin();
    display.clearDisplay();
    display.setTextColor(INKPLATE_BLACK);

    // Display the battery voltage
    displayBatteryVoltage();

    // Display an image from the SD card or the network
    success = sdCardDisplayImage();
    if (success == false)
        success = networkDisplayImage(NETWORK_TIMEOUT);

    // Display the image
    if (success == false)
    {
        Serial.println("ERROR: Failed to load the image");
        display.clearDisplay();
    }

    // Display the battery voltage at the bottom of the screen
    displayBatteryVoltageOverlay();

    // Display the image
    display.display();

    // Display the parameters
    Serial.printf("imageNumber: %d\r\n", imageNumber);
    Serial.printf("displayMinutes: %d\r\n", displayMinutes);
    Serial.printf("imagesBeforeCsvUpdate: %d\r\n", imagesBeforeCsvUpdate);
    Serial.printf("displayFiles: %d\r\n", displayFiles);

    // Display the time to next CSV update
    uint32_t minutes = (uint32_t)displayMinutes
                     * (uint32_t)(imagesBeforeCsvUpdate - (imageNumber % imagesBeforeCsvUpdate));
    uint32_t hours = minutes / 60;
    minutes -= hours * 60;
    Serial.printf("Time before CVS update: %d:%02d\r\n", hours, minutes);

    // Set the next image number
    imageNumber += 1;

    // Activate wakeup timer
    esp_sleep_enable_timer_wakeup((uint64_t)displayMinutes * 60ull * 1000ull * 1000ull);

    // Enter deep sleep
    deepSleep();
}

//----------------------------------------
// Display the battery voltage at the bottom of the screen
//----------------------------------------
void displayBatteryVoltageOverlay()
{
    uint16_t h;
    int16_t hRect;
    size_t length;
    uint16_t w;
    int16_t wRect;
    int16_t x;
    int16_t x1;
    int16_t y;
    int16_t y1;

    // Determine the text size
    display.setTextSize(2);
    display.getTextBounds(batteryVoltageString, 0, 0, &x1, &y1, &w, &h);

    // Add the white background for the text
    length = strlen(batteryVoltageString);
    hRect = (6 * h) / 4;
    wRect = ((length + 1) * w) / length;
    x = (DISPLAY_WIDTH - wRect) / 2;
    y = DISPLAY_HEIGHT - hRect;
    display.setCursor(x, y);
    display.fillRect(x, y, wRect, hRect, INKPLATE_WHITE);

    // Add the battery voltage text
    x = (DISPLAY_WIDTH - w) / 2;
    y = DISPLAY_HEIGHT - ((5 * h) / 4);
    display.setCursor(x, y);
    display.setTextColor(INKPLATE_BLACK);
    display.print(batteryVoltageString);
}

//----------------------------------------
// Enter deep sleep
//----------------------------------------
void deepSleep()
{
    // Enable push button wakup from deep sleep on gpio 18
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_18, 0);

    // Start deep sleep (this function does not return). Program stops here.
    Serial.print("Going to sleep\r\n\n\n");
    esp_deep_sleep_start();
}

//----------------------------------------
// loop - Called multiple times, each time executes a single pass
//----------------------------------------
void loop()
{
    // Never here! If you are using deep sleep, the whole program should be in setup() because the board restarts each
    // time. loop() must be empty!
}

//----------------------------------------
// Draw text centered on the line
//----------------------------------------
void drawCenteredText(int16_t x, int16_t y, const char * text)
{
    uint16_t h;
    uint16_t w;
    int16_t x1;
    int16_t y1;

    // Calculate bounds starting at dummy coordinate (0, 0)
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

    // Center the text horizontally
    x = (DISPLAY_WIDTH - w) / 2;
    display.setCursor(x, y);
    display.print(text);
}

//----------------------------------------
// Display the battery voltage
//----------------------------------------
void displayBatteryVoltage()
{
    float batteryVoltage;
    uint16_t h;
    const int loopCount = 8;
    uint16_t w;
    int16_t x;
    int16_t x1;
    int16_t y;
    int16_t y1;

    // Compute the voltage
    batteryVoltage = 0;
    for (int i = 0; i < loopCount; i++)
        batteryVoltage += display.readBattery(); // Read battery voltage
    batteryVoltage = batteryVoltage / loopCount;

    // Determine if it is time to charge the battery
    Serial.printf("Battery Voltage: %4.2fV\r\n", batteryVoltage);
    sprintf(batteryVoltageString, "Battery: %4.2fV", batteryVoltage);
    if (batteryVoltage < CHARGE_VOLTAGE)
    {
        x = 25;
        y = 0;
        int16_t vSpacing = 200;

        // Request USB cable
        y += vSpacing;
        display.clearDisplay();     // Clear the frame buffer (does NOT clear the physical screen)
        display.setTextSize(20);    // Set text size (uint8_t, default is 1)
        drawCenteredText(x, y, "Plug In");

        y += vSpacing;
        drawCenteredText(x, y, "USB Cable");

        // Display the voltage
        y += vSpacing;
        y += vSpacing;
        display.setTextSize(18);    // Set text size (uint8_t, default is 1)
        drawCenteredText(x, y, batteryVoltageString);

        // Refresh the e-paper display to show changes
        display.display();

        // Enter sleep mode
        deepSleep();
    }
}

//----------------------------------------
// Dump a buffer in hex and ASCII
//----------------------------------------
void dumpBuffer(size_t offset, const uint8_t *buffer, size_t length)
{
    int bytes;
    const uint8_t *end;
    int index;

    end = &buffer[length];
    while (buffer < end)
    {
        // Determine the number of bytes to display on the line
        bytes = end - buffer;
        if (bytes > (16 - (offset & 0xf)))
            bytes = 16 - (offset & 0xf);

        // Display the offset
        Serial.printf("0x%08lx: ", offset);

        // Skip leading bytes
        for (index = 0; index < (offset & 0xf); index++)
            Serial.printf("   ");

        // Display the data bytes
        for (index = 0; index < bytes; index++)
            Serial.printf("%02X ", buffer[index]);

        // Separate the data bytes from the ASCII
        for (; index < (16 - (offset & 0xf)); index++)
            Serial.printf("   ");
        Serial.printf(" ");

        // Skip leading bytes
        for (index = 0; index < (offset & 0xf); index++)
            Serial.printf(" ");

        // Display the ASCII values
        for (index = 0; index < bytes; index++)
            Serial.printf("%c", ((buffer[index] < ' ') || (buffer[index] >= 0x7f)) ? '.' : buffer[index]);
        Serial.printf("\r\n");

        // Set the next line of data
        buffer += bytes;
        offset += bytes;
    }
}

//----------------------------------------
// If debug option is on, print available heap
//----------------------------------------
void reportHeapNow(bool includePsram)
{
    if (includePsram)
        Serial.printf("FreeHeap: %d / HeapLowestPoint: %d / LargestBlock: %d / Used PSRAM: %d\r\n",
                      ESP.getFreeHeap(), xPortGetMinimumEverFreeHeapSize(),
                      heap_caps_get_largest_free_block(MALLOC_CAP_8BIT), ESP.getPsramSize() - ESP.getFreePsram());
    else
        Serial.printf("FreeHeap: %d / HeapLowestPoint: %d / LargestBlock: %d\r\n", ESP.getFreeHeap(),
                      xPortGetMinimumEverFreeHeapSize(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}
