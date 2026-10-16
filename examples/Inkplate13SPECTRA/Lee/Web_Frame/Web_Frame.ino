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

#define MICROSECONDS_IN_A_MILLISECOND   1000ull
#define MILLISECONDS_IN_A_SECOND        1000ull
#define SECONDS_IN_A_MINUTE             60ull

#define MICROSECONDS_IN_A_SECOND    (MICROSECONDS_IN_A_MILLISECOND * MILLISECONDS_IN_A_SECOND)
#define MICROSECONDS_IN_A_MINUTE    (MICROSECONDS_IN_A_SECOND * SECONDS_IN_A_MINUTE)

#define NETWORK_TIMEOUT             ((uint32_t)(15 * MICROSECONDS_IN_A_MINUTE))

//----------------------------------------
// Globals
//----------------------------------------

Inkplate display;
volatile RTC_DATA_ATTR long randomNumber;

#if 0
setup:
    determine if SD card available
    if (sd card available)
    {
        determine if USB is connected
        get load date from RTC
        if (current date != load date) || (.csv does not exist on SD card)
        {
            start network
            read .csv file
            copy it to SD card
            save load date to RTC
            get total files from RTC
            for each entry in the .csv file
            {
                if (file does not exist on SD card)
                {
                    read file from network
                    Save file to SD card
                    Add one to total files
                }
            }
            save total files to RTC
        }
        if (.csv file on SD card)
            displayImageFromSdCard()
        else
            displayImageFromNetwork();
    }
    else
        displayImageFromNetwork();
    generate new random number
    save random number in rtc
    enable wakeup_push_button
    enable wakeup_timer(15 minutes)
    deep sleep

displayImageFromNetwork()
{
    start network
    read .csv file
    get random number from RTC
    count total files listed in .csv
    choose url from .csv file
    display that image
}

displayImageFromSdCard()
{
    read .csv file from SD card
    get random number from RTC
    get total files from RTC
    choose image file from .csv file
    display that image
}

#endif // 0

//----------------------------------------
// Starting point for the application
//----------------------------------------
void setup()
{
    Serial.begin(115200);

    // Display a sign-on message
    Serial.println("Web_Frame application");

    // Initialize the display
    display.begin();
    display.setTextColor(INKPLATE_BLACK);

    // Display an image from the network
    networkDisplayImage(NETWORK_TIMEOUT, &Serial);

    // Generate and save the random number
Serial.printf("Seeding with randomNumber: %d\r\n", randomNumber);
    randomNumber = randomNumber + 1;
Serial.printf("randomNumber: %d\r\n", randomNumber);

    Serial.println("Going to sleep");

    // Enable push button wakup from deep sleep on gpio 18
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_18, 0);

    // Activate wakeup timer
    esp_sleep_enable_timer_wakeup(15ll * 60 * 1000 * 1000);

    // Start deep sleep (this function does not return). Program stops here.
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
