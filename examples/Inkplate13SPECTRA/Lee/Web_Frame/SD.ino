/**
 **************************************************
 * @file        SD.ino
 * @brief       SD card support routines for Web_Frame.
 *
 * @details     Shows how you can set the Inkplate to display random pictures
 *              from the SD card.
 *
 * @author      Lee Leahy
 * @date        2026-10-16
 **************************************************/

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
// SD card delete directory or directory tree
//----------------------------------------
bool sdCardDeleteDirectory(const char * path, bool walkTree, Print * debugPrint)
{
    bool success;

    success =  sdCardWalkDirectory(path,
                                   walkTree,
                                   sdCardDeleteFile,
                                   nullptr,
                                   debugPrint);
    if ((success == false) && debugPrint)
        sdCardDirectoryListing(path, walkTree, debugPrint);
    return success;
}

//----------------------------------------
// SD card delete a file
//----------------------------------------
bool sdCardDeleteFile(const FILE_INFO ** fileTable,
                      uint32_t fileCount,
                      void * context,
                      Print * debugPrint)
{
    char * filePath;
    SdFile sdRootDir;
    bool success;

    // Start at the beginning of the directory
    if (sdRootDir.open("/") == false)
    {
        Serial.println("ERROR: Failed to open the root directory");
        success = false;
    }
    else
    {
        // Walk the file table and delete the files
        success = true;
        for (uint32_t entry = 0; entry < fileCount; entry++)
        {
            // Skip the directory entries
            if (fileTable[entry]->_isDirectory == false)
            {
                SdFile file;
                filePath = (char *)&fileTable[entry]->_filePath[1];
                if (file.open(filePath, O_RDWR) == false)
                {
                    perror("Failed to open file\r\n");
                    Serial.printf("File: %s\r\n", filePath);
                    success = false;
                }
                if (file.remove() == false)
                {
                    perror("Failed to delete file\r\n");
                    Serial.printf("File: %s\r\n", filePath);
                    success = false;
                }
            }
        }

        // Walk the file table and delete the directories if possible
        for (uint32_t entry = 0; entry < fileCount; entry++)
        {
            // Skip the file entries
            if (fileTable[entry]->_isDirectory)
            {
                filePath = (char *)&fileTable[entry]->_filePath[1];
                filePath[strlen(filePath) - 1] = 0;
                if (display.getSdFat().rmdir(filePath) == false)
                {
                    perror("Failed to delete directory\r\n");
                    Serial.printf("File: %s\r\n", filePath);
                    success = false;
                }
            }
        }

        // Close the directory
        sdRootDir.close();
    }
    return success;
}

//----------------------------------------
// SD card directory listing
//----------------------------------------
bool sdCardDirectoryListFiles(const FILE_INFO ** fileTable,
                              uint32_t fileCount,
                              void * context,
                              Print * debugPrint)
{
    // Display the directory listing
    sdCardDisplayFileInfoTableHeader(fileCount);
    for (int entry = 0; entry < fileCount; entry++)
        sdCardDisplayFileInfoTableEntry(fileTable[entry]);
    return true;
}

//----------------------------------------
// SD card directory listing
//----------------------------------------
bool sdCardDirectoryListing(const char * path, bool walkTree, Print * debugPrint)
{
    return sdCardWalkDirectory(path,
                               walkTree,
                               sdCardDirectoryListFiles,
                               nullptr,
                               debugPrint);
}

//----------------------------------------
// Display a file table entry
//----------------------------------------
void sdCardDisplayFileInfoTableEntry(const FILE_INFO * fileInfo)
{
    // Display the file attributes
    Serial.printf("%10d   %s   %s\r\n",
                  fileInfo->_fileBytes,
                  fileInfo->_isDirectory ? "Dir" : "   ",
                  fileInfo->_filePath);
}

//----------------------------------------
// Display the file table header
//----------------------------------------
void sdCardDisplayFileInfoTableHeader(uint32_t fileCount)
{
    // Display the partition name
    Serial.printf("\r\nSD Card Files\r\n");

    if (fileCount == 0)
        Serial.printf("No SD card files found\r\n");
    else
    {
        Serial.printf("      Size   Dir   File Name\r\n");
        //                      1                  1         2
        //             1234567890   123   12345678901234567890
        Serial.printf("----------   ---   --------------------\r\n");
    }
}

//----------------------------------------
// Display an image from the SD card
//----------------------------------------
bool sdCardDisplayImage(Print * debugPrint)
{
    bool success;

    do
    {
        success = false;

        // Init SD card. Display if SD card is init propery or not.
        if (display.sdCardInit() == false)
        {
            if (debugPrint)
                debugPrint->println("Warning: Failed to initialize the SD card");
            break;
        }
        if (debugPrint)
            debugPrint->println("SD card online");

sdCardDeleteDirectory("/", true, &Serial);
sdCardDirectoryListing("/", true, &Serial);
    } while (0);
    return success;
}

/*
    // Init SD card. Display if SD card is init propery or not.
    if (display.sdCardInit())
    {
        Serial.println("SD Card OK! Reading image...");

        // Attempt to open the file
        if (file.open(fileName, O_READ) == false)
        {
            Serial.printf("ERROR: Failed to open SD card file %s\r\n", fileName);
            break;
        }

        // Display the file name
        Serial.printf("SD card file %s dump:\r\n", fileName);

        // Walk the contents of the file
        offset = 0;
        while (file.available())
        {
            // Read some data
            bytesRead = 0;
            do
            {
                int data = file.read();
                if (data < 0)
                {
                    if (bytesRead == 0)
                        bytesRead = data;
                    break;
                }
                buffer[bytesRead] = (uint8_t)data;
                bytesRead += 1;
            } while ((bytesRead > 0) && (bytesRead < bufferLength));
            if (bytesRead < 0)
            {
                Serial.printf("ERROR: Hard read error at offset %ld in SD file %s\r\n",
                              offset, fileName);
                break;
            }

            // Display the data
            dumpBuffer(offset, buffer, bytesRead);
            offset += bytesRead;
        }

        // Done with the file
        if (file)
            file.close();

    }
*/

//----------------------------------------
// SD card directory listing
//----------------------------------------
bool sdCardWalkDirectory(const char * path,
                         bool walkTree,
                         SD_CARD_PROCESS_FILES processFiles,
                         void * context,
                         Print * debugPrint)
{
    uint32_t fileCount;
    FILE_INFO * fileList;
    FILE_INFO ** fileTable;
    bool success;
    FILE_INFO * temp;

    // Start at the beginning of the directory
    fileList = nullptr;
    success = sdCardWalkDirectoryTree(path,
                                      fileList,
                                      walkTree,
                                      processFiles,
                                      context,
                                      debugPrint);
    if (success)
    {
        fileCount = 0;
        fileTable = nullptr;

        // Count the files in the list
        temp = fileList;
        while (temp)
        {
            fileCount += 1;
            temp = temp->_next;
        }

        // Build the table of files
        if (fileCount == 0)
            Serial.println("No files found");
        else
        {
            // Allocate the file table
            fileTable = (FILE_INFO **)malloc(fileCount * sizeof(fileTable));
            if (fileTable == nullptr)
                Serial.println("ERROR: Failed to allocate file table");
            else
            {
                // Initialize the file table
                temp = fileList;
                for (int entry = 0; entry < fileCount; entry += 1)
                {
                    fileTable[entry] = temp;
                    temp = temp->_next;
                }

                // Sort the file table
                for (int i = 0; i < (fileCount - 1); i++)
                {
                    for (int j = i + 1; j < fileCount; j++)
                    {
                        if (strcasecmp(fileTable[i]->_filePath, fileTable[j]->_filePath) > 0)
                        {
                            // Switch entries
                            temp = fileTable[i];
                            fileTable[i] = fileTable[j];
                            fileTable[j] = temp;
                        }
                    }
                }
            }
        }

        // Process the directory listing
        if (processFiles)
            success = processFiles((const FILE_INFO **)fileTable, fileCount, context, debugPrint);
    }

    // Free the file table
    if (fileTable)
        free(fileTable);

    // Free the list of files
    while (fileList)
    {
        temp = fileList;
        fileList = fileList->_next;
        free(temp);
    }
    return success;
}

//----------------------------------------
// SD card directory listing
//----------------------------------------
bool sdCardWalkDirectoryTree(const char * path,
                             FILE_INFO * &fileList,
                             bool walkTree,
                             SD_CARD_PROCESS_FILES processFiles,
                             void * context,
                             Print * debugPrint)
{
    bool directory;
    FILE_INFO * fileInfo;
    char fileName[256];
    SdFile sdFile;
    SdFile sdRootDir;
    size_t size;
    char * string;
    bool success;

    // Start at the beginning of the directory
    success = true;
    if (sdRootDir.open(path) == false)
    {
        Serial.printf("ERROR: Failed to open directory: %s\r\n", path);
        success = false;
    }
    else
    {
        // List the contents of the directory
        while (1)
        {
            // Open the next file
            if (!sdFile.openNext(&sdRootDir, O_RDONLY))
                break;

            // Get the file attributes
            sdFile.getName(fileName, sizeof(fileName));
            size = sdFile.fileSize();
            directory = sdFile.isDir();

            // Done with this file
            sdFile.close();

            // Allocate the FILE_INFO structure
            fileInfo = (FILE_INFO *)malloc(sizeof(FILE_INFO)
                                           + strlen(path)
                                           + strlen(fileName)
                                           + (directory ? 1 : 0)
                                           + 1);
            if (fileInfo == nullptr)
            {
                Serial.println("ERROR: Failed to allocate FILE_INFO structure");
                success = false;
                break;
            }

            // Fill the FILE_INFO object
            fileInfo->_filePath = (const char *)(fileInfo + 1);
            string = (char *)(fileInfo->_filePath);
            strcpy(string, path);
            strcat(string, fileName);
            if (directory)
                strcat(string, "/");
            fileInfo->_fileBytes = size;
            fileInfo->_isDirectory = directory;
            fileInfo->_next = fileList;
            fileList = fileInfo;

            // Walk the subdirectory
            if (walkTree)
            {
                if (sdCardWalkDirectoryTree(string,
                                            fileList,
                                            walkTree,
                                            processFiles,
                                            context,
                                            debugPrint) == false)
                {
                    success = false;
                    break;
                }
            }
        }

        // Close the directory
        sdRootDir.close();
    }
    return success;
}

#if 0
//----------------------------------------
// Copy the data stream to the SD file
//----------------------------------------
bool copyStreamToSdCardFile(const char * fileName,
                            size_t fileBytes,
                            Stream * stream,
                            Print * debugPrint)
{
    char * buffer;
    SdFile sdRootDir;

    // Start at the beginning of the directory
    if (sdRootDir.open("/") == false)
    {
        if (debugPrint)
            debugPrint->println("ERROR: Failed to open the root directory.");
        return false;
    }

    do
    {
        // Allocate the buffer
        buffer = malloc(16384);
        if (buffer == nullptr)
        {
            if (debugPrint)
                debugPrint->println("ERROR: Failed to allocate copy buffer.");
            break;
        }

        // Read the .CSV file
        char * data = buffer;
        char * dataEnd = &buffer[fileBytes];
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


    } while (0);

    // Done with the buffer
    if (buffer)
        free(buffer);

    // Close the directory
    sdRootDir.close();
}

//----------------------------------------
// Display an image from the network
//----------------------------------------
void copyCvsFileToSdCardTmpFile(uint32_t timeout, Print * debugPrint)
{
    char * csvFileData;
    NetworkClient * csvStream;
    size_t fileBytes;
    HTTPClient https;
    char * rawCsvData;
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

    char fileName[60];
    bool directory;
    SdFile sdFile;
    bool headerDisplayed;
    const char * name;
    size_t size;

    // Display the partition name
    Serial.printf("\nSD Card Files\r\n");

        // List the contents of the directory
        headerDisplayed = false;
        while (1)
        {
            // Open the next file
            if (!sdFile.openNext(&sdRootDir, O_RDONLY))
            {
                if (headerDisplayed == false)
                    Serial.printf("No SD card files found\r\n");
                break;
            }

            // Get the file attributes
            sdFile.getName(fileName, sizeof(fileName));
            size = sdFile.fileSize();
            directory = sdFile.isDir();

            // Done with this file
            sdFile.close();

            // Display the attributes
            if (headerDisplayed == false)
            {
                headerDisplayed = true;
                Serial.printf("      Size   Dir   File Name\r\n");
                //                      1                  1         2
                //             1234567890   123   12345678901234567890
                Serial.printf("----------   ---   --------------------\r\n");
            }

            // Display the file attributes
            Serial.printf("%10d   %s   %s\r\n", size, directory ? "Dir" : "   ", fileName);
        }

        // Close the directory
        sdRootDir.close();
    }

        // Allocate space for the raw CSV file
        rawCsvFile = (char *)malloc(fileBytes + 1);
        if (csvDrawCsvFileata == nullptr)
        {
            if (debugPrint)
                debugPrint->println("ERROR: Failed to allocate the rawCsvFile");
            break;
        }

        // Allocate space for the CSV file
        rawCsvData = (char *)malloc(fileBytes + 1);
        if (rawCsvData == nullptr)
        {
            if (debugPrint)
                debugPrint->println("ERROR: Failed to allocate the rawCsvData");
            break;
        }

        // Make sure the last line is terminated
        rawCsvData[fileBytes] = 0;

        // Allocate space for the CSV file
        csvFileData = (char *)malloc(fileBytes + 1);
        if (csvFileData == nullptr)
        {
            if (debugPrint)
                debugPrint->println("ERROR: Failed to allocate the csvFileData");
            break;
        }

        // Make sure the last line is terminated
        csvFileData[fileBytes] = 0;

        // Read the .CSV file
        char * data = rawCsvData;
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
        data = rawCsvData;
        dataEnd = &data[fileBytes];
        char * dataLines = csvFileData;
        while (data < dataEnd)
        {
            // Replace linefeeds with a NUL
            if (*data == '\n')
            {
                // Zero-terminate the line
                *dataLines++ = 0;

                // Count this line
                lineCount += 1;
            }
            else
                dataLines++ = *data;
            data += 1;
        }

        // Determine if the last line is terminated
        csvFileData -= 1;
        if (*csvFidataLinesleData != 0)
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
        data = csvFileData;
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
    if (csvFileData)
        free(csvFileData);
    https.end();
}

#endif  // 0
