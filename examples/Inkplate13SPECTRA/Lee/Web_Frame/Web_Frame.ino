/**
 **************************************************
 * @file        Inkplate13SPECTRA_Image_Frame_From_Web.ino
 * @brief       Web image frame example for Soldered Inkplate 13SPECTRA.
 *
 * @details     Shows how you can set the Inkplate to display random pictures
 *              from the web.
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
 * @author      Soldered
 * @date        2026-01-26
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
    displayImageFromNetwork(NETWORK_TIMEOUT, &Serial);

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
// Start the network
//----------------------------------------
void startTheNetwork(Print * debugPrint)
{
    // Attempt to connect to the network
    while (display.connectWiFiMulti(WIFI_SSID_ENTRIES, ssid, password) == false)
    {
    }

    // Display the network connection
    if (debugPrint)
    {
        debugPrint->print("Successfully connected to WiFi network ");
        debugPrint->print(WiFi.SSID());
        debugPrint->print(", ");
        debugPrint->print(WiFi.STA.localIP().toString().c_str());
        debugPrint->println("!");
        debugPrint->println("joined wifi");
    }
}

//----------------------------------------
// Extract the web server from the URL, without any port number
//----------------------------------------
String getServerFromUrl(const char * url)
{
    const char * http = "http://";
    const char * https = "https://";
    size_t index;
    size_t length;
    size_t pos;
    char * server;
    String serverString;

    do
    {
        // Verify that a URL was specified
        serverString = String("");
        if (url == nullptr)
        {
            Serial.println("ERROR: No URL specified!");
            break;
        }

        // Remove the protocol prefix from the URL
        index = 0;
        pos = 0;
        length = strlen(url);
        char server[length + 1];
        if (strncmp(url, https, strlen(https)) == 0)
            pos = strlen(https);
        else if (strncmp(url, http, strlen(http)) == 0)
            pos = strlen(http);
        if (pos)
        {
            // Locate the beginning of the server name
            strcpy(server, &url[pos]);

            // Walk the rest of the URL to locate the end of the server name
            for (index = 0; index < length - pos; index++)
            {
                // Check for end of URL
                if (server[index] == 0)
                    break;

                // Check for port specification or path specification
                if ((server[index] == ':') || (server[index] == '/'))
                    break;
            }
        }

        // Build the server name string
        server[index] = 0;
        serverString = server;
    } while (0);
    return serverString;
}

//----------------------------------------
// Get an IP address associated with server
//----------------------------------------
String getServerIpAddress(const char * server, Print * debugPrint)
{
    struct addrinfo hints, * res, * p;
    char ipstr[INET6_ADDRSTRLEN];
    String ipAddress;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // Support IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(server, NULL, &hints, &res);
    if (status != 0)
    {
        if (debugPrint)
            debugPrint->printf("getaddrinfo error: %d\r\n", status);
    }
    else
    {
        void * addr = nullptr;
        const char * ipVersion;

        for (p = res; p != NULL; p = p->ai_next)
        {
            // Check for an IPv4 address
            if (p->ai_family == AF_INET)
            {
                struct sockaddr_in * ipv4 = (struct sockaddr_in *)p->ai_addr;
                addr = &(ipv4->sin_addr);
                ipVersion = "IPv4";
                break;
            }

            // Check for an IPv6 address
            else if (p->ai_family == AF_INET6)
            {
                struct sockaddr_in6 * ipv6 = (struct sockaddr_in6 *)p->ai_addr;
                addr = &(ipv6->sin6_addr);
                ipVersion = "IPv6";
                break;
            }
        }

        if (addr)
        {
            inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
            ipAddress = String(ipstr);
        }

        freeaddrinfo(res); // Free the memory
    }
    return ipAddress;
}

//----------------------------------------
// Connect to the remote web server specified by the URL.
// Return the file length if possible.
//----------------------------------------
bool serverConnectUsingUrl(const char * url,
                           NetworkClient * &stream,
                           HTTPClient &https,
                           size_t &fileBytes,
                           Print * debugPrint)
{
    int attempt;
    const t_http_codes expectedResponseCode = HTTP_CODE_OK;
    int httpResponseCode;
    bool success;

    do
    {
        success = false;
        stream = nullptr;
        fileBytes = 0;

        // Locate the server for this URL
        String serverString = getServerFromUrl(url);
        if (serverString.length() == 0)
        {
            if (debugPrint)
                debugPrint->println("ERROR: Failed to find server name in URL string");
            break;
        }
        const char * server = serverString.c_str();

        // Translate the server name into an IP address
        String ipAddressString = getServerIpAddress(server, debugPrint);
        if (ipAddressString.length() == 0)
        {
            if (debugPrint)
                debugPrint->println("Failed to get the IP address for the server");
            break;
        }
        const char * ipAddress = ipAddressString.c_str();

        // Retry the connection up to 3 times
        const int attemptMax = 3;
        for (int attempt = 1; attempt <= attemptMax; attempt++)
        {
            bool beginSuccess;

            // Build the request for the web server
            beginSuccess = https.begin(url);
            if (beginSuccess == false)
            {
                if (debugPrint)
                    debugPrint->println("ERROR: Failed to set the URL for the web server!\r\n");
                break;
            }

            // Tell the HTTP layer to follow redirect links returned by the web server
            https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

            // Send the request to the web server and get the web server's response
            httpResponseCode = https.GET();
            if (httpResponseCode >= 0)
                break;

            // Display the error message
            if (debugPrint)
                debugPrint->printf("ERROR: HTTP GET failed, attempt %d of 3: %d (%s)\r\n",
                                   attempt, httpResponseCode,
                                   https.errorToString(httpResponseCode).c_str());

            if (attempt < 3)
                delay(500);

            // Handle the error
            https.end();
        }
        if (attempt > attemptMax)
            break;

        // Display the error
        if ((httpResponseCode == expectedResponseCode) && debugPrint)
            debugPrint->printf("HTTP Response code: %d (%s)\r\n", httpResponseCode,
                               https.errorToString(httpResponseCode).c_str());

        // Handle the error from the web server
        if (httpResponseCode != expectedResponseCode)
        {
            if (debugPrint)
                debugPrint->printf("Web server response %d: %s\r\n", httpResponseCode,
                                   https.errorToString(httpResponseCode).c_str());
            if ((httpResponseCode == HTTP_CODE_OK) && debugPrint)
                // A 200 here means the server ignored our Range request and is about to send
                // the whole file from byte 0 - streaming that into this offset would corrupt
                // the image, so bail rather than guess.
                debugPrint->printf("HTTP range request failed, code: %d, fileBytes: %d\r\n",
                                   httpResponseCode, https.getSize());
            break;
        }

        // Get the file size
        fileBytes = https.getSize();
        if (debugPrint)
            debugPrint->printf("File size: %d (0x%08x) bytes\r\n", fileBytes, fileBytes);
        if ((ssize_t)fileBytes <= 0)
        {
            debugPrint->println("ERROR: Web server did not report a file size.");
            break;
        }

        // Get the stream
        stream = https.getStreamPtr();
        success = true;
    } while (0);

    return success;
}

//----------------------------------------
// Display an image from the network
//----------------------------------------
void displayImageFromNetwork(uint32_t timeout, Print * debugPrint)
{
    char * csvData;
    NetworkClient * csvStream;
    size_t fileBytes;
    HTTPClient https;
    size_t remainingBytes;
    char * url;

    do
    {
        // Start the network
        startTheNetwork(debugPrint);

        // Attempt to open the .CSV file
        if (serverConnectUsingUrl(csvFileUrl,
                                  csvStream,
                                  https,
                                  fileBytes,
                                  debugPrint) == false)
        {
            break;
        }

        // Allocate space for the CSV file
        csvData = (char *)malloc(fileBytes + 1);
        if (csvData == nullptr)
        {
            if (debugPrint)
                debugPrint->println("ERROR: Failed to allocate the csvStream");
            break;
        }

        // Make sure the last line is terminated
        csvData[fileBytes] = 0;

        // Read the .CSV file
        char * data = csvData;
        char * dataEnd = &data[fileBytes];
        uint32_t timerStart = millis();
        while (data < dataEnd)
        {
            // Wait for some data
            size_t bytesToRead = csvStream->available();
            if (bytesToRead == 0)
            {
                // Check for timeout
                if ((millis() - timerStart) < timeout)
                {
                    if (debugPrint)
                        debugPrint->println("ERROR: Timed out reading .CSV file");
                    break;
                }
                delay(1);
                continue;
            }

            // Read data into the buffer
            ssize_t bytesRead = csvStream->read((uint8_t *)data, bytesToRead);
            if (bytesRead < 0)
            {
                if (debugPrint)
                    debugPrint->println("ERROR: Failed to read .CSV file!");
                break;
            }

            // Account for this data
            data += bytesRead;
        }
        if (data < dataEnd)
            break;

        // Separate and count the lines in the .csv file
        uint32_t lineCount = 0;
        data = csvData;
        dataEnd = &data[fileBytes];
        while (data < dataEnd)
        {
            // Replace linefeeds with a NUL
            if (*data == '\n')
            {
                // Zero-terminate the line
                *data = 0;

                // Count this line
                lineCount += 1;
            }
            data += 1;
        }

        // Determine if the last line is terminated
        data -= 1;
        if (*data != 0)
        {
            // Zero-terminate the last line
            *data = 0;

            // Count this line
            lineCount += 1;
        }

        // Check for an empty file
        if (lineCount == 0)
        {
            if (debugPrint)
                debugPrint->println("ERROR: Empty .CSV found, no image to display!");
            break;
        }

        // Check for a file with just a header
        if (lineCount == 1)
        {
            if (debugPrint)
                debugPrint->println("ERROR: Header line found but no image to display!");
            break;
        }

        // Select a line in the file
        uint32_t lineNumber = randomNumber % (lineCount - 1);

        // Skip the header line
        lineNumber += 1;

        // Locate the line
        data = csvData;
        while (lineNumber--)
            data += strlen((const char *)data) + 1;

        // Locate the URL
        while (*data++ != ',')
        {
        }
        url = data;

        // Display the image
        displayUrl(url);
    } while (0);

    // Release the resources
    if (csvData)
        free(csvData);
    https.end();
}


void displayUrl(char * url)
{
    // Get the final image URL (resolve HTTP redirect)
    Serial.println(url);

    // Locate the image extension
    char * extension = &url[strlen(url) -1];
    while (*extension != '.')
        extension -= 1;

    // Select routine based upon the image extension
    if (strcmp(extension, ".bmp") == 0)
    {
        Serial.println("Loading .BMP image");
        Serial.println(display.image.drawBitmapFromWeb(url, 0, 0, true, false));
    }
    else if ((strcmp(extension, ".jpg") == 0) || (strcmp(extension, ".jpeg") == 0))
    {
        // Download and draw the JPEG image directly from the web
        // Function returns 1 on success and 0 on failure
        Serial.println("Loading .JPG image");
        Serial.println(display.image.drawJpegFromWeb(url, 0, 0, true, false));
    }
    else if (strcmp(extension, ".png") == 0)
    {
        Serial.println("Loading .PNG image");
        Serial.println(display.image.drawPngFromWeb(url, 0, 0, true, false));
    }
    display.display();
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
