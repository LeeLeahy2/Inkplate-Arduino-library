/**
 **************************************************
 * @file        Network.ino
 * @brief       Network support routines for Web_Frame
 *
 * @details     Shows how you can set the Inkplate to display random pictures
 *              from the web.
 *
 * @author      Lee Leahy
 * @date        2026-10-16
 **************************************************/

//----------------------------------------
// Display the CSV table
//----------------------------------------
void displayCsvTable(const CSV_ENTRY * csvTable, uint32_t lineCount)
{
    const CSV_ENTRY * entry;
    const char * line = "Line";
    const char * name = "File Path";
    const char * url = "URL";
    int width;
    int widthLine;
    int widthName;
    int widthUrl;
    int x;
    int y;

    // Determine the column widths
    entry = csvTable;
    widthLine = strlen(line);
    widthName = strlen(name);
    widthUrl = strlen(url);
    for (int i = 0; i < lineCount; i++)
    {
        // Determine the file path width
        entry = &csvTable[i];
        width = strlen(entry->_filePath);
        if (width > widthName)
            widthName = width;

        // Determine the url width
        width = strlen(entry->_url);
        if (width > widthUrl)
        widthUrl = width;
    }

    // Display the CSV table top header line
    displayCsvTableHeaderLine(widthLine, widthName, widthUrl, '.');

    // Display the line column header
    Serial.printf("|%s|", line);

    // Display the filePath column header
    y = widthName - strlen(name);
    x = y / 2;
    for (int i = 0; i < x; i++)
        Serial.write(' ');
    Serial.print(name);
    x = y - x;
    for (int i = 0; i < x; i++)
        Serial.write(' ');
    Serial.write('|');

    // Display the URL column header
    y = widthUrl - strlen(url);
    x = y / 2;
    for (int i = 0; i < x; i++)
        Serial.write(' ');
    Serial.print(url);
    x = y - x;
    for (int i = 0; i < x; i++)
        Serial.write(' ');
    Serial.write('|');
    Serial.print("\r\n");

    // Display the CSV table bottom header line
    displayCsvTableHeaderLine(widthLine, widthName, widthUrl, '|');

    // Display the CSV table entries
    entry = csvTable;
    for (int i = 0; i < lineCount; i++)
    {
        Serial.write('|');

        // Display the line number
        y = widthLine - 2;
        x = y / 2;
        for (int i = 0; i < x; i++)
            Serial.write(' ');
        Serial.printf("%2d", i);
        x = y - x;
        for (int i = 0; i < x; i++)
            Serial.write(' ');
        Serial.write('|');

        // Display the filePath
        y = widthName - strlen(csvTable[i]._filePath);
        Serial.printf(csvTable[i]._filePath);
        for (int i = 0; i < y; i++)
            Serial.write(' ');
        Serial.write('|');

        // Display the URL
        y = widthUrl - strlen(csvTable[i]._url);
        Serial.printf(csvTable[i]._url);
        for (int i = 0; i < y; i++)
            Serial.write(' ');
        Serial.write('|');

        // Done with this line
        Serial.print("\r\n");
    }

    // Display the CSV table bottom line
    displayCsvTableHeaderLine(widthLine, widthName, widthUrl, '\'');
}

//----------------------------------------
// Display a CSV header line
//----------------------------------------
void displayCsvTableHeaderLine(int widthLine,
                               int widthName,
                               int widthUrl,
                               char separator)
{
    // Display a separator line
    Serial.write(separator);
    for (int j = 0; j < widthLine; j++)
        Serial.write('-');
    Serial.write(separator);
    for (int j = 0; j < widthName; j++)
        Serial.write('-');
    Serial.write(separator);
    for (int j = 0; j < widthUrl; j++)
        Serial.write('-');
    Serial.write(separator);
    Serial.println();
}

//----------------------------------------
// Display the lines
//----------------------------------------
void displayLines(const char * data, size_t bytes, uint32_t lineNumber)
{
    const char * dataEnd;

    dataEnd = &data[bytes];
    while (data < dataEnd)
    {
        Serial.printf("%2d) %s\r\n", lineNumber++, data);
        data += strlen(data) + 1;
    }
}

// Build the csvTable
//----------------------------------------
bool networkBuildCsvTable(char * csvData,
                          size_t fileBytes,
                          uint32_t lineCount,
                          CSV_ENTRY ** csvTable)
{
    CSV_ENTRY * csvEntry;
    char * data;
    char * dataEnd;
    bool success;

    do
    {
        success = false;

        // Allocate the CSV table
        *csvTable = (CSV_ENTRY *)malloc(lineCount * sizeof(CSV_ENTRY));
        if (csvTable == nullptr)
        {
            Serial.println("ERROR: Failed to allocate CSV table");
            break;
        }

        // Walk the lines in the CSV file
        csvEntry = *csvTable;
        data = csvData;
        dataEnd = &data[fileBytes];
        while (data < dataEnd)
        {
            // Skip comment lines
            if (*data == COMMENT_CHARACTER)
            {
                data += strlen(data) + 1;
                continue;
            }

            // Save the beginning of the line
            csvEntry->_filePath = data;

            // Locate the comma separation
            while (*data)
            {
                if (*data == ',')
                {
                    // Zero-terminate the file path
                    *data++ = 0;
                    break;
                }
                data += 1;
            }

            // Save the URL
            csvEntry->_url = data;

            // Get the next line and csvEntry
            data += strlen(data) + 1;
            csvEntry += 1;
        }
        success = true;
    } while (0);
    return success;
}

//----------------------------------------
// SD card copy the network files
//----------------------------------------
bool NetworkCopyFilesToSdCard(uint32_t timeout)
{
    bool success;

    // Load, parse and sort the CSV file
    success = networkReadCsvFile(sdCardWriteTemporaryCsvFile,
                                 sdCardCopyNetworkFiles,
                                 timeout);
    if (debugPrint)
        debugPrint->printf("NetworkCopyFilesToSdCard returning %d\r\n", success);
    return success;
}

//----------------------------------------
// Count the lines in the CSV file
//----------------------------------------
uint32_t networkCountCsvLines(char * csvData, size_t fileBytes)
{
    char * data;
    char * dataEnd;
    uint32_t lineCount;

    // Count the lines in the .csv file, skipping comment lines
    lineCount = 0;
    data = csvData;
    dataEnd = &data[fileBytes];
    while (data < dataEnd)
    {
        // Count this line
        if (*data != COMMENT_CHARACTER)
            lineCount += 1;

        // Set the start of the next line
        data += strlen(data) + 1;
    }
    return lineCount;
}

//----------------------------------------
// Display an image from the network
//----------------------------------------
bool networkDisplayImage(uint32_t timeout)
{
    bool success;

    // Load, parse and sort the CSV file
    success = networkReadCsvFile(nullptr, networkSelectImage, timeout);
    return success;
}

//----------------------------------------
// Display the contents of the URL
//----------------------------------------
bool networkDisplayUrl(const char * url)
{
    bool success;

    // Display the URL
    Serial.println(url);

    // Locate the image extension
    const char * extension = &url[strlen(url) -1];
    while (*extension != '.')
        extension -= 1;

    // Select routine based upon the image extension
    success = false;
    if (strcmp(extension, ".bmp") == 0)
    {
        Serial.println("Loading .BMP image");
        success = display.image.drawBitmapFromWeb(url, 0, 0, true, false);
    }
    else if ((strcmp(extension, ".jpg") == 0) || (strcmp(extension, ".jpeg") == 0))
    {
        // Download and draw the JPEG image directly from the web
        // Function returns 1 on success and 0 on failure
        Serial.println("Loading .JPG image");
        success = display.image.drawJpegFromWeb(url, 0, 0, true, false);
    }
    else if (strcmp(extension, ".png") == 0)
    {
        Serial.println("Loading .PNG image");
        success = display.image.drawPngFromWeb(url, 0, 0, true, false);
    }

    if (debugPrint)
        debugPrint->printf("networkDisplayUrl returning %d\r\n", success);
    return success;
}

//----------------------------------------
// Extract the web server from the URL, without any port number
//----------------------------------------
String networkGetServerFromUrl(const char * url)
{
    const char * http = "http://";
    const char * https = "https://";
    size_t index;
    size_t length;
    size_t pos;
    char server[strlen(url) + 1];
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
        server[0] = 0;
        if (strncasecmp(url, https, strlen(https)) == 0)
            pos = strlen(https);
        else if (strncasecmp(url, http, strlen(http)) == 0)
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
String networkGetServerIpAddress(const char * server)
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
// Get the parameters from the CSV file
//----------------------------------------
char * networkParseCsvFileParameters(char * csvData, size_t fileBytes)
{
    char * data;
    char * dataEnd;
    char errorMessage[80];
    uint32_t lineNumber;
    bool parameterFound[csvParameterTableEntries];
    const char * parameterName;
    uint32_t parameterNumber;
    int parameterType;
    char * parameterValue;
    bool skipParameter;
    int value;

    // No parameters found yet
    for (parameterNumber = 0; parameterNumber < csvParameterTableEntries; parameterNumber++)
        parameterFound[parameterNumber] = false;

    // Walk through the parameter area in the CSV file
    data = csvData;
    dataEnd = &data[fileBytes];
    lineNumber = 1;
    endOfParameters = false;
    while (data < dataEnd)
    {
        char * nextLine;

        // Skip over the comments
        if (*data == COMMENT_CHARACTER)
        {
            data += strlen(data) + 1;
            lineNumber += 1;
            continue;
        }

        // Separate the parameter name from the parameter value
        parameterName = data;
        parameterValue = data;
        nextLine = &data[strlen(data) + 1];
        while (*parameterValue != ',')
            parameterValue += 1;
        *parameterValue++ = 0;

        // Attempt to locate the parameter
        skipParameter = false;
        for (parameterNumber = 0; parameterNumber < csvParameterTableEntries; parameterNumber++)
        {
            if (strcmp(parameterName, csvParameterTable[parameterNumber]._parameterName) == 0)
                break;
        }

        // Display an error is the parameter was not found
        if (parameterNumber >= csvParameterTableEntries)
        {
            sprintf(errorMessage, "Line %d, unknown parameter: %s", lineNumber, parameterName);
            Serial.printf("%s\r\n", errorMessage);
            reportFatalError(errorMessage);
        }
        // Set the value
        else if (parameterSetValue(parameterNumber, parameterValue))
            // Mark the parameter as found
            parameterFound[parameterNumber] = true;

        // Set the next line
        data = nextLine;
        if (endOfParameters)
            break;
    }

    // Set the default values
    for (parameterNumber = 0; parameterNumber < csvParameterTableEntries; parameterNumber++)
    {
        const char * defaultValue;
        if (parameterFound[parameterNumber] == false)
        {
            defaultValue = csvParameterTable[parameterNumber]._parameterValue;
            parameterSetValue(parameterNumber, defaultValue);
        }
    }

    // Return the list of file names and URLs
    return data;
}

//----------------------------------------
// Read the CSV file from the network
//----------------------------------------
bool networkReadCsvFile(SD_CARD_WRITE_CSV_FILE writeCsvFile,
                        NETWORK_PROCESS_CSV_LINES processLines,
                        uint32_t timeout)
{
    int count;
    NetworkClient * csvStream;
    size_t fileBytes;
    HTTPClient https;
    bool success;
    const char * wifiPass[WIFI_SSID_ENTRIES];
    const char * wifiSsid[WIFI_SSID_ENTRIES];

    // Remove empty entries
    count = 0;
    for (int i = 0; i < WIFI_SSID_ENTRIES; i++)
    {
        if ((ssid[i] != nullptr) && (ssid[i][0] != 0))
        {
            wifiSsid[count] = ssid[i];
            wifiPass[count] = password[i];
            count += 1;
        }
    }

    // Verify that the CSV file is specified and that at least one WiFi
    // access point SSID and password are specified
    if ((csvFileUrl == nullptr) || (csvFileUrl[0] == 0))
        Serial.printf("Enter csvFileUrl and then reboot\r\n");
    if (count == 0)
        Serial.printf("Enter WiFi SSID and password and then reboot\r\n");

    // Wait for required NVM parameters
    if ((count == 0)
        || (csvFileUrl == nullptr) || (csvFileUrl[0] == 0))
    {
        Serial.printf("Waiting for parameter entry and reboot\r\n");
        while ((count == 0)
            || (csvFileUrl == nullptr) || (csvFileUrl[0] == 0))
        {
        }
    }

    // Start the network
    networkStart(count, wifiSsid, wifiPass);

    // Attempt to open the .CSV file
    success = networkServerConnectUsingUrl(csvFileUrl,
                                           csvStream,
                                           https,
                                           fileBytes);
    if (success)
        // Read and process the CSV file
        success = networkReadCsvFileFromStream("network",
                                               csvStream,
                                               fileBytes,
                                               writeCsvFile,
                                               processLines,
                                               timeout);
    https.end();
    return success;
}

//----------------------------------------
// Read the CSV file from a stream
//----------------------------------------
bool networkReadCsvFileFromStream(const char * device,
                                  Stream * csvStream,
                                  size_t fileBytes,
                                  SD_CARD_WRITE_CSV_FILE writeCsvFile,
                                  NETWORK_PROCESS_CSV_LINES processLines,
                                  uint32_t timeout)
{
    char * csvData;
    CSV_ENTRY * csvEntry;
    CSV_ENTRY * csvTable;
    CSV_ENTRY * csvTemp;
    uint32_t lineCount;
    size_t parameterBytes;
    size_t remainingBytes;
    bool success;
    char * urlData;

    do
    {
        success = false;
        csvData = nullptr;
        csvTable = nullptr;

        // Allocate space for the CSV file
        csvData = (char *)malloc(fileBytes + 1);
        if (csvData == nullptr)
        {
            Serial.println("ERROR: Failed to allocate the csvStream");
            break;
        }

        // Make sure the last line is terminated
        csvData[fileBytes] = 0;

        // Read the .CSV file
        Serial.printf("Reading CSV file from %s\r\n", device);
        if (networkReadCsvFileIntoDataBuffer(csvStream,
                                             csvData,
                                             fileBytes,
                                             timeout) == false)
            break;

        // Write the .CSV file to the SD card
        if (writeCsvFile && writeCsvFile(csvData, fileBytes) == false)
            break;

        // Separate the lines in the .csv file
        networkSeparateCsvLines(csvData, fileBytes);

        // Display the contents of the CSV file
        if (displayCsvFile)
        {
            Serial.println("------------------------------------------------");
            Serial.println("CSV file");
            Serial.println("------------------------------------------------");
            displayLines(csvData, fileBytes, 1);
        }

        // Get the parameters from the CSV file
        urlData = networkParseCsvFileParameters(csvData, fileBytes);
        parameterBytes = urlData - csvData;
        remainingBytes = fileBytes - parameterBytes;

        // Display the remaining data lines
        if (displayRawUrls)
        {
            Serial.println("------------------------------------------------");
            Serial.println("File path and URL data from CSV file");
            Serial.println("------------------------------------------------");
            displayLines(urlData, remainingBytes, 0);
        }

        // Count the lines in the .csv file, skipping comment lines
        lineCount = networkCountCsvLines(urlData, remainingBytes);

        // Build the csvTable
        if (networkBuildCsvTable(urlData,
                                 remainingBytes,
                                 lineCount,
                                 &csvTable) == false)
            break;

        // Display the CSV table
        if (displayUnsortedUrls)
            displayCsvTable(csvTable, lineCount);

        // Sort the entries
        for (int i = 0; i < (lineCount - 1); i++)
        {
            for (int j = i + 1; j < lineCount; j++)
            {
                if (strcasecmp(csvTable[i]._filePath, csvTable[j]._filePath) > 0)
                {
                    const char * temp;

                    // Switch entries
                    temp = csvTable[i]._filePath;
                    csvTable[i]._filePath = csvTable[j]._filePath;
                    csvTable[j]._filePath = temp;

                    temp = csvTable[i]._url;
                    csvTable[i]._url = csvTable[j]._url;
                    csvTable[j]._url = temp;
                }
            }
        }

        // Display the CSV table
        if (displaySortedUrls)
            displayCsvTable(csvTable, lineCount);

        // Process the CSV file
        if (processLines)
            success = processLines(csvTable, lineCount);
    } while (0);

    // Release the resources
    if (csvTable)
        free(csvTable);
    if (csvData)
        free(csvData);
    return success;
}

//----------------------------------------
// Read in the CSV file
//----------------------------------------
bool networkReadCsvFileIntoDataBuffer(Stream * csvStream,
                                      char * csvData,
                                      size_t fileBytes,
                                      uint32_t timeout)
{
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
            if ((millis() - timerStart) > timeout)
            {
                Serial.println("ERROR: Timed out reading .CSV file");
                break;
            }
            delay(1);
            continue;
        }

        // Read data into the buffer
        ssize_t bytesRead = csvStream->readBytes((uint8_t *)data, bytesToRead);
        if (bytesRead < 0)
        {
            Serial.println("ERROR: Failed to read .CSV file!");
            break;
        }

        // Account for this data
        data += bytesRead;
    }
    return (data == dataEnd);
}

//----------------------------------------
// Select and display an image from the network
//----------------------------------------
bool networkSelectImage(const CSV_ENTRY * csvTable, uint32_t lineCount)
{
    const CSV_ENTRY * entry;
    uint32_t lineNumber;
    bool success;
    int temp;
    const char * url;

    do      // Common exit
    {
        success = false;

        // Select a line in the file
        lineNumber = selectImage(lineCount);

        // Locate the URL
        url = csvTable[lineNumber]._url;

        // Display the image
        Serial.printf("Selected URL: %d) Net:%s\r\n", lineNumber, url);
        uint32_t startMsec = millis();
        success = networkDisplayUrl(url);
        if (success)
        {
            uint32_t milliseconds = millis() - startMsec;
            uint32_t seconds = milliseconds / MILLISECONDS_IN_A_SECOND;
            milliseconds -= seconds * MILLISECONDS_IN_A_SECOND;
            Serial.printf("%d.%03d seconds to write the frame buffer\r\n",
                          seconds, milliseconds);
        }
    } while (0);
    return success;
}

//----------------------------------------
// Separate the lines in the CSV file
//----------------------------------------
void networkSeparateCsvLines(char * csvData, size_t fileBytes)
{
    char * data;
    char * dataEnd;

    // Separate and count the lines in the .csv file, skipping comment lines
    data = csvData;
    dataEnd = &data[fileBytes];
    while (data < dataEnd)
    {
        // Replace linefeeds with a NUL
        if (*data == '\n')
            // Zero-terminate the line
            *data = 0;
        data += 1;
    }

    // Determine if the last line is terminated
    data -= 1;
    if (*data != 0)
    {
        // Zero-terminate the last line
        *data = 0;
    }
}

//----------------------------------------
// Connect to the remote web server specified by the URL.
// Return the file length if possible.
//----------------------------------------
bool networkServerConnectUsingUrl(const char * url,
                                  NetworkClient * &stream,
                                  HTTPClient &https,
                                  size_t &fileBytes)
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
        String serverString = networkGetServerFromUrl(url);
        if (serverString.length() == 0)
        {
            Serial.printf("URL: %s\r\n", url);
            Serial.println("ERROR: Failed to find server name in URL string");
            break;
        }
        const char * server = serverString.c_str();

        // Translate the server name into an IP address
        String ipAddressString = networkGetServerIpAddress(server);
        if (ipAddressString.length() == 0)
        {
            Serial.println("Failed to get the IP address for the server");
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
                Serial.println("ERROR: Failed to set the URL for the web server!\r\n");
                break;
            }

            // Tell the HTTP layer to follow redirect links returned by the web server
            https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

            // Send the request to the web server and get the web server's response
            httpResponseCode = https.GET();
            if (httpResponseCode >= 0)
                break;

            // Display the error message
            Serial.printf("ERROR: HTTP GET failed, attempt %d of 3: %d (%s)\r\n",
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
            Serial.printf("Web server response %d: %s\r\n", httpResponseCode,
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
            Serial.println("ERROR: Web server did not report a file size.");
            break;
        }

        // Get the stream
        stream = https.getStreamPtr();
        success = true;
    } while (0);

    return success;
}

//----------------------------------------
// Start the network
//----------------------------------------
void networkStart(int count, const char ** wifiSsid, const char ** wifiPass)
{
    // Attempt to connect to the network
    Serial.println("Starting WiFi network");
    while (display.connectWiFiMulti(count, wifiSsid, wifiPass) == false)
    {
    }

    // Display the network connection
    Serial.print("Successfully connected to WiFi network ");
    Serial.print(WiFi.SSID());
    Serial.print(", ");
    Serial.print(WiFi.STA.localIP().toString().c_str());
    Serial.println("!");
}

//----------------------------------------
// Select the image
//----------------------------------------
uint32_t selectImage(uint32_t lineCount)
{
    // Select a line in the file
    return imageNumber % lineCount;
}
