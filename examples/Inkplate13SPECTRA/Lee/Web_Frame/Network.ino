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
// Display an image from the network
//----------------------------------------
bool networkDisplayImage(uint32_t timeout, Print * debugPrint)
{
    bool success;

    // Load, parse and sort the CSV file
    success = networkReadCsvFile(networkSelectImage,
                                 timeout,
                                 debugPrint);
    return success;
}

//----------------------------------------
// Display the contents of the URL
//----------------------------------------
bool networkDisplayUrl(const char * url)
{
    bool success;

    // Get the final image URL (resolve HTTP redirect)
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

    // Display the image
    if (success)
        display.display();
    return success;
}

//----------------------------------------
// Read the CSV file from the network
//----------------------------------------
bool networkReadCsvFile(NETWORK_PROCESS_CSV_LINES processLines,
                        uint32_t timeout,
                        Print * debugPrint)
{
    char * csvData;
    CSV_ENTRY * csvEntry;
    CSV_ENTRY * csvTable;
    CSV_ENTRY * csvTemp;
    NetworkClient * csvStream;
    size_t fileBytes;
    HTTPClient https;
    size_t remainingBytes;
    bool success;
    const char * url;

    do
    {
        success = false;
        csvTable = nullptr;

        // Start the network
        networkStart(debugPrint);

        // Attempt to open the .CSV file
        if (networkServerConnectUsingUrl(csvFileUrl,
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

        // Allocate the CSV table
        csvTable = (CSV_ENTRY *)malloc(lineCount * sizeof(CSV_ENTRY));
        if (csvTable == nullptr)
        {
            if (debugPrint)
                debugPrint->println("ERROR: Failed to allocate CSV table");
            break;
        }

        // Walk the lines in the CSV file
        csvEntry = csvTable;
        data = csvData;
        dataEnd = &data[fileBytes];
        while (data < dataEnd)
        {
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

        // Sort the entries
        for (int i = 1; i < (lineCount - 1); i++)
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

        // Process the CSV file
        if (processLines)
            success = processLines(csvTable, lineCount, debugPrint);
    } while (0);

    // Release the resources
    if (csvData)
        free(csvData);
    https.end();
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
String networkGetServerIpAddress(const char * server, Print * debugPrint)
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
// Select and display an image from the network
//----------------------------------------
bool networkSelectImage(const CSV_ENTRY * csvTable,
                        uint32_t lineCount,
                        Print * debugPrint)
{
    uint32_t lineNumber;
    const char * url;

    // Select a line in the file
    lineNumber = imageNumber % (lineCount - 1);

    // Skip the header line
    lineNumber += 1;

    // Locate the URL
    url = csvTable[lineNumber]._url;

    // Display the image
    return networkDisplayUrl(url);
}

//----------------------------------------
// Connect to the remote web server specified by the URL.
// Return the file length if possible.
//----------------------------------------
bool networkServerConnectUsingUrl(const char * url,
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
        String serverString = networkGetServerFromUrl(url);
        if (serverString.length() == 0)
        {
            if (debugPrint)
                debugPrint->println("ERROR: Failed to find server name in URL string");
            break;
        }
        const char * server = serverString.c_str();

        // Translate the server name into an IP address
        String ipAddressString = networkGetServerIpAddress(server, debugPrint);
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
// Start the network
//----------------------------------------
void networkStart(Print * debugPrint)
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
    }
}
