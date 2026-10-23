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

//----------------------------------------
// Constants
//----------------------------------------

#define STR_CMP         strcasecmp

//----------------------------------------
// Globals
//----------------------------------------

uint8_t sectorBuffer[512] __attribute__ ((aligned (4)));
uint32_t cardSectorCount = 0;
SdCard * sdCard = nullptr;

//----------------------------------------
// SD card copy the network files
//----------------------------------------
bool sdCardCompareFiles(const FILE_INFO ** fileTable,
                        uint32_t fileCount,
                        void * context)
{
    const CSV_CONTEXT * csvContext;
    const CSV_ENTRY * csvTable;
    bool displayHeader;
    const CSV_ENTRY * entry;
    const FILE_INFO * fileInfo;
    uint32_t fileNumber;
    uint32_t lineCount;
    uint32_t lineNumber;
    bool success;

    // Expand the context
    displayHeader = true;
    success = true;
    csvContext = (CSV_CONTEXT *)context;
    csvTable = csvContext->_csvTable;
    lineCount = csvContext->_lineCount;

    if (displayFiles)
    {
        // List the files in the CSV file
        Serial.println("----------------------------------------");
        Serial.println("CSV Data");
        Serial.println("----------------------------------------");
        Serial.printf("lineCount: %d\r\n", lineCount);
        for (int i = 0; i < lineCount; i++)
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
    }

    // Copy the files to the SD card
    do
    {
        int temp;

        if (lineCount == 0)
        {
            break;
        }

        // Walk the list of files
        fileNumber = 0;
        lineNumber = HEADER_LINE_COUNT;
        entry = &csvTable[lineNumber];
        fileInfo = fileCount ? fileTable[fileNumber] : nullptr;
        Serial.println("Verifying files are on SD card");
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
                // Skip over the leading /
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
                                                     fileBytes) == false)
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
                    Serial.printf("Copy Net:%s -> SD:%s\r\n", entry->_url, entry->_filePath);

                    // Open the file on the SD card
                    if (!file.open(entry->_filePath, O_WRONLY | O_CREAT | O_TRUNC))
                    {
                        Serial.printf("Error opening SD:%s\r\n", entry->_filePath);
                        success = false;
                        break;
                    }

                    do      // Close file upon exit
                    {
                        // Display the header
                        if (displayHeader)
                        {
                            Serial.println("Copying file to the SD card");
                            displayHeader = false;
                        }

                        // Write content to the file
                        Serial.printf("Writing to SD:%s...\r\n", entry->_filePath);
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
                                    Serial.printf("ERROR: Timed out reading Net:%s\r\n", entry->_filePath);
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
                                Serial.printf("ERROR: Failed to read Net:%s\r\n", entry->_filePath);
                                success = false;
                                break;
                            }

                            // Write the data to the SD card
                            ssize_t bytesWritten = file.write((uint8_t *)buffer, bytesRead);
                            if (bytesWritten < 0)
                            {
                                Serial.printf("ERROR: Failed to write SD:%s\r\n", entry->_filePath);
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
                                debugPrint->printf("Removing file : SD:%s\r\n", entry->_filePath);
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
                Serial.printf("%s is already on SD card!\r\n", entry->_filePath);

            // Get the next CSV file entry
            entry = (++lineNumber < lineCount) ? entry + 1 : nullptr;
        }

        // Open the file on the SD card
        if (success)
        {
            // Rename the CSV file
            display.getSdFat().remove(CSV_FILE);
            display.getSdFat().rename(TEMPORARY_CSV_FILE, CSV_FILE);
        }
    } while (0);
    if (debugPrint)
        debugPrint->printf("sdCardCompareFiles returning %d\r\n", success);
    return success;
}

//----------------------------------------
// Copy files from the network to the SD card
//----------------------------------------
bool sdCardCopyNetworkFiles(const CSV_ENTRY * csvTable, uint32_t lineCount)
{
    CSV_CONTEXT csvContext;
    bool success;

    // Save the csvTable
    csvContext._csvTable = csvTable;
    csvContext._lineCount = lineCount;

    // Get the list of files on the SD card
    success = sdCardWalkDirectory("/",
                                  true,
                                  sdCardCompareFiles,
                                  (void *)&csvContext);
    if (debugPrint)
        debugPrint->printf("sdCardCopyNetworkFiles returning %d\r\n", success);
    return success;
}

//----------------------------------------
// SD card delete directory or directory tree
//----------------------------------------
bool sdCardDeleteDirectory(const char * path, bool walkTree)
{
    bool success;

    success =  sdCardWalkDirectory(path,
                                   walkTree,
                                   sdCardDeleteFile,
                                   nullptr);
    if ((success == false) && debugPrint)
        sdCardDirectoryListing(path, walkTree);
    return success;
}

//----------------------------------------
// SD card delete a file
//----------------------------------------
bool sdCardDeleteFile(const FILE_INFO ** fileTable,
                      uint32_t fileCount,
                      void * context)
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
                              void * context)
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
bool sdCardDirectoryListing(const char * path, bool walkTree)
{
    return sdCardWalkDirectory(path,
                               walkTree,
                               sdCardDirectoryListFiles,
                               nullptr);
}

//----------------------------------------
// Display the contents of the URL
//----------------------------------------
bool sdCardDisplayFile(const char * filePath)
{
    SdFile file;
    bool success;

    // Display the file path
    Serial.printf("Opening SD:%s\r\n", filePath);

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

        // Done with the file
        file.close();
    }
    if (debugPrint)
        debugPrint->printf("sdCardDisplayFile returning %d\r\n", success);
    return success;
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
bool sdCardDisplayImage(uint32_t timeout)
{
    uint64_t deltaBytes;
    uint64_t gBytes;
    uint64_t mBytes;
    uint64_t sdCardBytes;
    uint32_t sectorCount;
    bool success;

    do      // Single exit point
    {
        success = true;

        // Init SD card. Display if SD card is init propery or not.
        if (display.sdCardInit() == false)
        {
            Serial.println("Warning: Failed to initialize the SD card");
            success = false;
            break;
        }

        // Get the SD card object
        sdCard = display.getSdFat().card();

        do      // Discontinue use of SD card
        {
            // Display the SD card size
            sectorCount = sdCard->sectorCount();
            sdCardBytes = sectorCount * 512ull;
            gBytes = (sdCardBytes + (512ull * 1000ull * 1000ull)) / (1000ull * 1000ull * 1000ull);
            Serial.printf("%lldGB SD card\r\n", gBytes, mBytes);
            if (debugPrint)
            {
                gBytes = sdCardBytes / (1000ull * 1000ull * 1000ull);
                deltaBytes = sdCardBytes - (gBytes * 1000ull * 1000ull * 1000ull);
                mBytes = deltaBytes / (1000ull * 1000ull);
                Serial.printf("Actual SD card size: %lld.%03lldGB\r\n", gBytes, mBytes);
            }

            // Determine if the files should be updated
            bool csvExists = display.getSdFat().exists(CSV_FILE);
            if (((imageNumber % imagesBeforeCsvUpdate) == 0) || (csvExists == false))
                NetworkCopyFilesToSdCard(timeout);

            // Determine if the .CSV file exists
            if (csvExists == false)
            {
                // Display the file from the network
                Serial.println("CSV file not found on SD card");
                success = false;
                break;
            }

            // Read the Pictures.csv file
            success = sdCardReadPicturesCsvFile(sdCardSelectFile, timeout);
        } while (0);

        // Done using SD card
        display.sdCardSleep();
    } while (0);
    if (debugPrint)
        debugPrint->printf("sdCardDisplayImage returning %d\r\n", success);
    return success;
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
        if (!sdCard->erase(firstBlock, lastBlock))
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

    if (!sdCard->readSector(0, sectorBuffer))
    {
        Serial.println("ERROR: readBlock");
        sdCardErrorHalt();
    }
    success = true;
    Serial.printf("All data set to 0x%02x\r\n", sectorBuffer[0]);
    Serial.println("Erase done");
    return success;
}

//----------------------------------------
// Handle SD card fatal errors
//----------------------------------------
void sdCardErrorHalt()
{
    if (!sdCard)
    {
        Serial.println("ERROR: Invalid SD_CONFIG");
    }
    else if (sdCard->errorCode())
    {
        if (sdCard->errorCode() == SD_CARD_ERROR_CMD0)
        {
            Serial.println("ERROR: No card, wrong chip select pin, or wiring error?");
        }
        uint8_t errorCode = sdCard->errorCode();
        Serial.printf("SD errorCode = 0x%08x, ", errorCode);
        printSdErrorSymbol(&Serial, errorCode);
        Serial.print(", ");
        printSdErrorText(&Serial, errorCode);
        Serial.println();

        Serial.printf("SD errorData = %d\r\n", sdCard->errorData());
    }
    while (true)
    {
    }
}

//----------------------------------------
// Read the CSV file from the network
//----------------------------------------
bool sdCardReadPicturesCsvFile(NETWORK_PROCESS_CSV_LINES processLines,
                               uint32_t timeout)
{
    SdFile csvFile;
    size_t fileBytes;
    bool success;

    do
    {
        success = false;

        // Attempt to open the .CSV file
        Serial.printf("Opening SD:%s\r\n", CSV_FILE);
        if (!csvFile.open(CSV_FILE, O_RDONLY))
        {
            Serial.printf("Error opening SD:%s\r\n", CSV_FILE);
            break;
        }

        // Get the file size
        fileBytes = (size_t)csvFile.fileSize();

        // Read the CSV file
        SdFatStream fileStream = SdFatStream(&csvFile);
        success = networkReadCsvFileFromStream((Stream *)&fileStream,
                                               fileBytes,
                                               nullptr,
                                               processLines,
                                               timeout);
    } while (0);

    csvFile.close();
    if (debugPrint)
        debugPrint->printf("sdCardReadPicturesCsvFile returning %d\r\n", success);
    return success;
}

//----------------------------------------
// Select and display an image from the network
//----------------------------------------
bool sdCardSelectFile(const CSV_ENTRY * csvTable, uint32_t lineCount)
{
    const CSV_ENTRY * entry;
    const char * filePath;
    uint32_t lineNumber;
    bool success;

    if (displayFiles)
    {
        // List the files in the CSV file
        Serial.println("----------------------------------------");
        Serial.println("CSV Data");
        Serial.println("----------------------------------------");
        Serial.printf("lineCount: %d\r\n", lineCount);
        for (int i = 0; i < lineCount; i++)
        {
            entry = &csvTable[i];
            Serial.printf("%s,   %s\r\n", entry->_filePath, entry->_url);
        }
    }

    // Select a line in the file
    lineNumber = imageNumber % (lineCount - HEADER_LINE_COUNT);

    // Skip the header line
    lineNumber += HEADER_LINE_COUNT;

    // Locate the file path
    filePath = csvTable[lineNumber]._filePath;

    // Display the image
    Serial.printf("Selected file: SD:%s\r\n", filePath);
    uint32_t startMsec = millis();
    success = sdCardDisplayFile(filePath);
    if (success)
    {
        uint32_t milliseconds = millis() - startMsec;
        uint32_t seconds = milliseconds / MILLISECONDS_IN_A_SECOND;
        milliseconds -= seconds * MILLISECONDS_IN_A_SECOND;
        Serial.printf("%d.%03d seconds to write the frame buffer\r\n",
                      seconds, milliseconds);
    }
    if (debugPrint)
        debugPrint->printf("sdCardSelectFile returning %d\r\n", success);
    return success;
}

//----------------------------------------
// SD card directory listing
//----------------------------------------
bool sdCardWalkDirectory(const char * path,
                         bool walkTree,
                         SD_CARD_PROCESS_FILES processFiles,
                         void * context)
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
                                      context);
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
            success = processFiles((const FILE_INFO **)fileTable, fileCount, context);
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
                             void * context)
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
                                            context) == false)
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

//----------------------------------------
// Write the CSV data to a temporary file
//----------------------------------------
bool sdCardWriteTemporaryCsvFile(const char * csvData, size_t fileBytes)
{
    ssize_t bytesWritten;
    FsFile file;
    size_t offset;
    bool success;

    do
    {
        success = false;
        Serial.printf("Writing file %s\r\n", TEMPORARY_CSV_FILE);
        if (!file.open(TEMPORARY_CSV_FILE, O_WRONLY | O_CREAT | O_TRUNC))
        {
            Serial.printf("Error opening %s\r\n", TEMPORARY_CSV_FILE);
            break;
        }

        offset = 0;
        while (offset < fileBytes)
        {
            size_t bytesToWrite = min((size_t)32768, fileBytes - offset);

            // Write the temporary file
            bytesWritten = file.write(&csvData[offset], bytesToWrite);
            if (bytesWritten < 0)
            {
                Serial.printf("ERROR: Failed to write %s!\r\n", TEMPORARY_CSV_FILE);
                break;
            }

            // Account for this block of data
            offset += bytesWritten;
        }

        // Close the .CSV file
        file.close();
        success = true;
    } while (0);
    return success;
}
