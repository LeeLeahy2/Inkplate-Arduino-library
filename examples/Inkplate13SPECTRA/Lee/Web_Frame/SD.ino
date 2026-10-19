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

#define STR_CMP         strcasecmp

uint8_t sectorBuffer[512] __attribute__ ((aligned (4)));
uint32_t cardSectorCount = 0;
SdCard* m_card = nullptr;

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
// SD card copy the network files
//----------------------------------------
bool sdCardCompareFiles(const FILE_INFO ** fileTable,
                                       uint32_t fileCount,
                                       void * context,
                                       Print * debugPrint)
{
    const CSV_CONTEXT * csvContext;
    const CSV_ENTRY * csvTable;
    const CSV_ENTRY * entry;
    const FILE_INFO * fileInfo;
    uint32_t fileNumber;
    uint32_t lineCount;
    uint32_t lineNumber;
    bool success;

    // Expand the context
    success = true;
    csvContext = (CSV_CONTEXT *)context;
    csvTable = csvContext->_csvTable;
    lineCount = csvContext->_lineCount;

    // List the files in the CSV file
    Serial.println("----------------------------------------");
    Serial.println("CSV Data");
    Serial.println("----------------------------------------");
    Serial.printf("lineCount: %d\r\n", lineCount);
    for (int i = 1; i < lineCount; i++)
    {
        entry = &csvTable[i];
        Serial.printf("%s,   %s\r\n", entry->_filePath, entry->_url);
    }

    Serial.println("----------------------------------------");
    Serial.println("SD Card Files");
    Serial.println("----------------------------------------");
    Serial.printf("fileCount: %d\r\n", fileCount);
    for (int i = 0; i < fileCount; i++)
    {
        fileInfo = fileTable[i];
        Serial.printf("%s%s   %d bytes\r\n",
                      fileInfo->_filePath,
                      fileInfo->_isDirectory ? " (Dir)" : "",
                      fileInfo->_fileBytes);
    }

    // Copy the files to the SD card
    if (lineCount)
    {
        // Walk the list of files
        fileNumber = 0;
        lineNumber = 1;
        entry = &csvTable[lineNumber];
        fileInfo = fileCount ? fileTable[fileNumber] : nullptr;
        while (entry)
        {
            int diff;

            // Skip files that exist on the SD card and are not in the CSV list
            // or match a file on the CSV list
            while (fileInfo && (STR_CMP(&fileInfo->_filePath[1], entry->_filePath) < 0))
                fileInfo = (++fileNumber < fileCount) ? fileTable[fileNumber] : nullptr;

            // Determine if the file should be copied
            // > File does not exist on the SD card
            // = File exists on the SD card, verify file size, re-copy if necessary
            diff = 1;
            if (fileInfo)
                diff = STR_CMP(&fileInfo->_filePath[1], entry->_filePath);
            if (diff >= 0)
            {
                NetworkClient * dataStream;
                FsFile file;
                size_t fileBytes;
                HTTPClient https;

                // Copy the file to the SD card
                do      // Break web server connection upon exit
                {
                    // Attempt to open the network file
                    if (networkServerConnectUsingUrl(entry->_url,
                                                     dataStream,
                                                     https,
                                                     fileBytes,
                                                     debugPrint) == false)
                    {
                        success = false;
                        break;
                    }

                    // Check for the file on the SD card
                    if (diff == 0)
                    {
                        // Verify the file size
                        if (fileBytes == fileInfo->_fileBytes)
                            break;

                        // Wrong file size, copy the file again
                        Serial.printf("Wrong file size, SD file size %ld, expecting %ld bytes\r\n",
                                      fileInfo->_fileBytes, fileBytes);
                    }

                    // Display the file copy
                    if (debugPrint)
                        debugPrint->printf("Copy %s -> %s\r\n", entry->_url, entry->_filePath);

                    // Open the file on the SD card
                    if (!file.open(entry->_filePath, O_RDWR | O_CREAT | O_TRUNC))
                    {
                        Serial.printf("Error opening %s\r\n", entry->_filePath);
                        success = false;
                        break;
                    }

                    do      // Close file upon exit
                    {
                        // Write content to the file
                        Serial.printf("Writing to %s...\r\n", entry->_filePath);
                        static char buffer[16384];
                        size_t bytesRemaining = fileBytes;
                        uint32_t timerStart = millis();
                        while (bytesRemaining)
                        {
                            // Wait for some data
                            size_t bytesToRead = dataStream->available();
                            if (bytesToRead == 0)
                            {
                                // Check for timeout
                                if ((millis() - timerStart) > DATA_TIMEOUT)
                                {
                                    if (debugPrint)
                                        debugPrint->printf("ERROR: Timed out reading .CSV file\r\n", entry->_filePath);
                                    success = false;
                                    break;
                                }
                                delay(1);
                                continue;
                            }

                            // Read data into the buffer
                            bytesToRead = min(bytesToRead, sizeof(buffer));
                            ssize_t bytesRead = dataStream->read((uint8_t *)buffer, bytesToRead);
                            if (bytesRead < 0)
                            {
                                if (debugPrint)
                                    debugPrint->printf("ERROR: Failed to read %s\r\n", entry->_filePath);
                                success = false;
                                break;
                            }

                            // Write the data to the SD card
                            ssize_t bytesWritten = file.write((uint8_t *)buffer, bytesRead);
                            if (bytesWritten < 0)
                            {
                                if (debugPrint)
                                    debugPrint->println("ERROR: Failed to write to SD card!");
                                break;
                            }

                            // Account for this data
                            bytesRemaining -= bytesRead;
                        }

                        // Delete the file upon error
                        if (bytesRemaining || (file.fileSize() != fileBytes))
                        {
                            if (debugPrint)
                            {
                                if ((bytesRemaining == 0) && (file.fileSize() != fileBytes))
                                    debugPrint->printf("Wrong file size, SD file size: %lld, expected %ld bytes\r\n",
                                                       file.fileSize(), fileBytes);
                                debugPrint->printf("Removing file : %s\r\n", entry->_filePath);
                            }
                            file.remove();
                            break;
                        }
                        if (debugPrint)
                            debugPrint->println("Done");
                    } while (0);

                    // Done with the file
                    file.close();
                } while (0);

                // Done with the web server
                https.end();
            }
            else if (STR_CMP(&fileInfo->_filePath[1], entry->_filePath) == 0)
                Serial.printf("%s already on SD card!\r\n", entry->_filePath);

            // Get the next CSV file entry
            entry = (++lineNumber < lineCount) ? entry + 1 : nullptr;
        }

        // Open the file on the SD card
        if (success)
        {
            FsFile file;
            if (!file.open("Pictures.csv", O_RDWR | O_CREAT | O_TRUNC))
            {
                Serial.printf("Error opening %s\r\n", entry->_filePath);
                success = false;
            }
            else
            {
                // Add the CSV file to the SD card
                lineNumber = 1;
                entry = &csvTable[lineNumber];

                // Rebuild the contents of the .CSV file
                while (entry)
                {
                    // Write the data to the SD card
                    ssize_t bytesWritten = file.printf("%s,%s\n", entry->_filePath, entry->_url);
                    if (bytesWritten < 0)
                    {
                        if (debugPrint)
                            debugPrint->println("ERROR: Failed to write to SD card!");
                        success = false;
                        break;
                    }

                    // Get the next CSV file entry
                    entry = (++lineNumber < lineCount) ? entry + 1 : nullptr;
                }

                // Close the .CSV file
                file.close();
            }
        }
    }
    Serial.printf("sdCardCompareFiles returning %d\r\n", success);
    return success;
}

//----------------------------------------
// Copy files from the network to the SD card
//----------------------------------------
bool sdCardCopyNetworkFiles(const CSV_ENTRY * csvTable,
                            uint32_t lineCount,
                            Print * debugPrint)
{
    CSV_CONTEXT csvContext;
    bool success;

    // Save the csvTable
    csvContext._csvTable = csvTable;
    csvContext._lineCount = lineCount;

    // Get the list of files on the SD card
reportHeapNow(false);
    success = sdCardWalkDirectory("/",
                                  true,
                                  sdCardCompareFiles,
                                  (void *)&csvContext,
                                  debugPrint);
reportHeapNow(false);
    Serial.printf("sdCardCopyNetworkFiles returning %d\r\n", success);
    return success;
}

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
// Handle SD card fatal errors
//----------------------------------------
void sdCardErrorHalt()
{
    if (!m_card)
    {
        Serial.println("ERROR: Invalid SD_CONFIG");
    }
    else if (m_card->errorCode())
    {
        if (m_card->errorCode() == SD_CARD_ERROR_CMD0)
        {
            Serial.println("ERROR: No card, wrong chip select pin, or wiring error?");
        }
        uint8_t errorCode = m_card->errorCode();
        Serial.printf("SD errorCode = 0x%08x, ", errorCode);
        printSdErrorSymbol(&Serial, errorCode);
        Serial.print(", ");
        printSdErrorText(&Serial, errorCode);
        Serial.println();

        Serial.printf("SD errorData = %d\r\n", m_card->errorData());
    }
    while (true)
    {
    }
}

//----------------------------------------
// flash erase all data
//----------------------------------------
uint32_t const ERASE_SIZE = 262144L;
bool sdCardEraseCard()
{
    bool success;
    Serial.println("Erasing");
    uint32_t firstBlock = 0;
    uint32_t lastBlock;
    uint16_t n = 0;

    success = false;
    do
    {
        lastBlock = firstBlock + ERASE_SIZE - 1;
        if (lastBlock >= cardSectorCount)
        {
            lastBlock = cardSectorCount - 1;
        }
        if (!m_card->erase(firstBlock, lastBlock))
        {
            Serial.println("ERROR: erase failed");
            sdCardErrorHalt();
        }
        Serial.print(".");
        if ((n++) % 64 == 63)
        {
            Serial.println();
        }
        firstBlock += ERASE_SIZE;
    } while (firstBlock < cardSectorCount);
    Serial.println();

    if (!m_card->readSector(0, sectorBuffer))
    {
        Serial.println("ERROR: readBlock");
        sdCardErrorHalt();
    }
    success = true;
    Serial.printf("All data set to  0x%02x\r\n", sectorBuffer[0]);
    Serial.println("Erase done");
    return success;
}

//----------------------------------------
// Display an image from the SD card
//----------------------------------------
bool sdCardDisplayImage(Print * debugPrint)
{
    bool success;

    do
    {
        success = true;

        // Init SD card. Display if SD card is init propery or not.
        if (display.sdCardInit() == false)
        {
            if (debugPrint)
                debugPrint->println("Warning: Failed to initialize the SD card");
            success = false;
            break;
        }
        if (debugPrint)
            debugPrint->println("SD card online");

/*
static SdCardFactory cardFactory;
ExFatFormatter exFatFormatter;
FatFormatter fatFormatter;
#define SD_CONFIG SdSpiConfig(10, SHARED_SPI, SD_SCK_MHZ(25))

Serial.println("Configuring the SD card\r\n");
m_card = cardFactory.newCard(SD_CONFIG);
uint32_t sectorCount = m_card->sectorCount();
uint64_t sdCardBytes = sectorCount * 512ull;
Serial.printf("SD card sector count: %ld\r\n", sectorCount);
uint64_t gBytes = sdCardBytes / (1000ull * 1000ull * 1000ull);
uint64_t deltaBytes = sdCardBytes - (gBytes * 1000ull * 1000ull * 1000ull);
uint64_t mBytes = deltaBytes / (1000ull * 1000ull);
Serial.printf("sdCardBytes: %13lld\r\n", sdCardBytes);
Serial.printf("deltaBytes: %13lld\r\n", deltaBytes);
Serial.printf("gBytes: %13lld\r\n", gBytes);
Serial.printf("mBytes: %13lld\r\n", mBytes);
Serial.printf("SD card size: %lld.%03lld GB\r\n", gBytes, mBytes);

if (!m_card || m_card->errorCode())
success = false;
if (success)
{
Serial.println("Erasing the SD card");
success = sdCardEraseCard();
if (success)
{
Serial.println("Formatting the SD card");
// Format exFAT if larger than 32GB.
success = cardSectorCount > 67108864
? exFatFormatter.format(m_card, sectorBuffer, &Serial)
: fatFormatter.format(m_card, sectorBuffer, &Serial);
}
}

//success = display.getSdFat().format(&Serial); //sdCardFormat();
while (success == false)
{
    Serial.println("SD card format failed!");
    delay(15 * 1000);
}
*/

//reportHeapNow(false);
//sdCardDeleteDirectory("/", true, &Serial);
reportHeapNow(false);
        // Determine if the files should be updated
        if (((imageNumber % 12) == 0) || display.getSdFat().exists("Pictures.csv"))
            NetworkCopyFilesToSdCard(NETWORK_TIMEOUT, &Serial);

        // Determine if the .CSV file exists
        if (display.getSdFat().exists("Pictures.csv") == false)
        {
            // Display the file from the network
            success = false;
            break;
        }

        // Read the Pictures.csv file
        success = sdCardReadPicturesCsvFile(sdCardSelectFile, &Serial);



/*
reportHeapNow(false);
sdCardDirectoryListing("/", true, &Serial);
reportHeapNow(false);
*/
    } while (0);
    Serial.printf("sdCardDisplayImage returning %d\r\n", success);
    return success;
}

//----------------------------------------
// Read the CSV file from the network
//----------------------------------------
bool sdCardReadPicturesCsvFile(NETWORK_PROCESS_CSV_LINES processLines,
                               Print * debugPrint)
{
    char * csvData;
    CSV_ENTRY * csvEntry;
    SdFile csvFile;
    CSV_ENTRY * csvTable;
    CSV_ENTRY * csvTemp;
    size_t fileBytes;
    const char * filePath = "Pictures.csv";
    size_t remainingBytes;
    bool success;
    const char * url;

    do
    {
        success = false;
        csvData = nullptr;
        csvTable = nullptr;

        // Attempt to open the .CSV file
        if (!csvFile.open(filePath, O_RDONLY))
        {
            Serial.printf("Error opening %s\r\n", filePath);
            success = false;
            break;
        }

        // Allocate space for the CSV file
        fileBytes = (size_t)csvFile.fileSize();
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

        // Read data into the buffer
        ssize_t bytesRead = csvFile.read((uint8_t *)data, fileBytes);
        if (bytesRead < 0)
        {
            if (debugPrint)
                debugPrint->println("ERROR: Failed to read .CSV file!");
            break;
        }

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
    if (csvTable)
        free(csvTable);
    if (csvData)
        free(csvData);
    csvFile.close();
    Serial.printf("sdCardReadPicturesCsvFile returning %d\r\n", success);
    return success;
}

//----------------------------------------
// Select and display an image from the network
//----------------------------------------
bool sdCardSelectFile(const CSV_ENTRY * csvTable,
                      uint32_t lineCount,
                      Print * debugPrint)
{
    const char * filePath;
    uint32_t lineNumber;
    bool success;

    // Select a line in the file
    lineNumber = imageNumber % (lineCount - 1);

    // Skip the header line
    lineNumber += 1;

    // Locate the file path
    filePath = csvTable[lineNumber]._filePath;

    // Display the image
    success = sdCardDisplayFile(filePath);
    Serial.printf("sdCardSelectFile returning %d\r\n", success);
    return success;
}

//----------------------------------------
// Display the contents of the URL
//----------------------------------------
bool sdCardDisplayFile(const char * filePath)
{
    SdFile file;
    bool success;

    // Display the file path
    Serial.println(filePath);

    // Open the file
    success = false;
    if (file.open(filePath, O_RDONLY) == false)
        Serial.printf("Failed to open file %s\r\n", filePath);
    else
    {
        // Locate the image extension
        const char * extension = &filePath[strlen(filePath) -1];
        while (*extension != '.')
            extension -= 1;

        // Select routine based upon the image extension
        if (strcmp(extension, ".bmp") == 0)
        {
            Serial.println("Loading .BMP image");
            success = display.image.drawBitmapFromSd(&file, 0, 0, false, false);
        }
        else if ((strcmp(extension, ".jpg") == 0) || (strcmp(extension, ".jpeg") == 0))
        {
            // Download and draw the JPEG image directly from the web
            // Function returns 1 on success and 0 on failure
            Serial.println("Loading .JPG image");
            success = display.image.drawJpegFromSd(&file, 0, 0, false, false);
        }
        else if (strcmp(extension, ".png") == 0)
        {
            Serial.println("Loading .PNG image");
            success = display.image.drawPngFromSd(&file, 0, 0, false, false);
        }

        // Display the image
        if (success)
            display.display();
        else
            Serial.println("Failed to load the image");

        // Done with the file
        file.close();
    }
    Serial.printf("sdCardDisplayFile returning %d\r\n", success);
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
// Format the SD card
//----------------------------------------
bool sdCardFormat()
{
}

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
                if ((millis() - timerStart) > timeout)
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
                if ((millis() - timerStart) > timeout)
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
