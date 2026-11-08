/**
 **************************************************
 * @file        NVM.ino
 * @brief       Non-Volatile Memory (NVM) support routines for Web_Frame
 *
 * @details     Code copied from R4A_ESP32 library.
 *
 * @author      Lee Leahy
 * @date        2026-11-07
 **************************************************/

//****************************************
// Constants
//****************************************

const char * r4aEsp32NvmTypeTable[] =
{
    "nullptr",      //  0
    "bool",         //  1
    "int8_t",       //  2
    "uint8_t",      //  3
    "int16_t",      //  4
    "uint16_t",     //  5
    "int32_t",      //  6
    "uint32_t",     //  7
    "int64_t",      //  8
    "uint64_t",     //  9
    "float",        // 10
    "double",       // 11
    "const char *", // 12
};

//****************************************
// Globals
//****************************************

bool r4aEsp32NvmDebug; // Set to true to enable debug output

//*********************************************************************
// Support routines
//*********************************************************************
// Parse the value parameter
// Inputs:
//   parameter: Address of the specified parameter in the table
//   valueString: Character string containing the new value
//   value: Address to receive the new value
//   display: Device used for output
// Outputs:
//   Returns true if the value was set and false otherwise
bool r4aEsp32NvmParseValue(const R4A_ESP32_NVM_PARAMETER * parameter,
                           const char * valueString,
                           R4A_ESP32_NVM_VALUE * value,
                           Print * display)
{
    bool goodMaxValue;
    bool goodMinValue;
    int length;
    int maximum;
    int minimum;
    char * newValue;
    bool uintValue;
    bool valid;

    // Determine the parameter type
    valid = false;
    goodMaxValue = true;
    goodMinValue = true;
    uintValue = true;
    value->u64 = 0;
    switch (parameter->type)
    {
    default:
        if (display)
            display->printf("ERROR: Unknown parameter type (%d) for %s\r\n", parameter->type, parameter->name);
        r4aReportFatalError("r4aEsp32NvmParseValue: Invalid parameter type!");
        break;

    case R4A_ESP32_NVM_PT_BOOL:
    case R4A_ESP32_NVM_PT_INT8:
    case R4A_ESP32_NVM_PT_UINT8:
    case R4A_ESP32_NVM_PT_INT16:
    case R4A_ESP32_NVM_PT_UINT16:
    case R4A_ESP32_NVM_PT_INT32:
    case R4A_ESP32_NVM_PT_UINT32:
    case R4A_ESP32_NVM_PT_INT64:
    case R4A_ESP32_NVM_PT_UINT64:
        // Convert the string into a numeric value
        valid = ((sscanf(valueString, "0x%llx", &value->u64) == 1)
                || (sscanf(valueString, "%llu", &value->u64) == 1));
        if (valid)
        {
            // Validate the value
            switch (parameter->type)
            {
            case R4A_ESP32_NVM_PT_BOOL:
                goodMaxValue = (value->u64 <= parameter->maximum);
                goodMinValue = (value->u64 >= parameter->minimum);
                break;

            case R4A_ESP32_NVM_PT_INT8:
                goodMaxValue = (value->i64 <= (int64_t)parameter->maximum);
                goodMinValue = (value->i64 >= (int64_t)parameter->minimum);
                uintValue = false;
                break;

            case R4A_ESP32_NVM_PT_UINT8:
                goodMaxValue = (value->u64 <= parameter->maximum);
                goodMinValue = (value->u64 >= parameter->minimum);
                break;

            case R4A_ESP32_NVM_PT_INT16:
                goodMaxValue = (value->i64 <= (int64_t)parameter->maximum);
                goodMinValue = (value->i64 >= (int64_t)parameter->minimum);
                uintValue = false;
                break;

            case R4A_ESP32_NVM_PT_UINT16:
                goodMaxValue = (value->u64 <= parameter->maximum);
                goodMinValue = (value->u64 >= parameter->minimum);
                break;

            case R4A_ESP32_NVM_PT_INT32:
                goodMaxValue = (value->i64 <= (int64_t)parameter->maximum);
                goodMinValue = (value->i64 >= (int64_t)parameter->minimum);
                uintValue = false;
                break;

            case R4A_ESP32_NVM_PT_UINT32:
                goodMaxValue = (value->u64 <= parameter->maximum);
                goodMinValue = (value->u64 >= parameter->minimum);
                break;

            case R4A_ESP32_NVM_PT_INT64:
                goodMaxValue = (value->i64 <= (int64_t)parameter->maximum);
                goodMinValue = (value->i64 >= (int64_t)parameter->minimum);
                uintValue = false;
                break;

            case R4A_ESP32_NVM_PT_UINT64:
                goodMaxValue = (value->u64 <= parameter->maximum);
                goodMinValue = (value->u64 >= parameter->minimum);
                break;
            }
            valid = goodMinValue & goodMaxValue;
            if (!valid)
            {
                if (display && (!goodMaxValue))
                {
                    if (uintValue)
                        display->printf("ERROR: Bad maximum value: %lld > %lld\r\n", value->u64, parameter->maximum);
                    else
                        display->printf("ERROR: Bad maximum value: %lld < %lld\r\n", value->i64, (int64_t)parameter->maximum);
                }
                if (display && (!goodMinValue))
                {
                    if (uintValue)
                        display->printf("ERROR: Bad minimum value: %lld > %lld\r\n", value->u64, parameter->minimum);
                    else
                        display->printf("ERROR: Bad minimum value: %lld < %lld\r\n", value->i64, (int64_t)parameter->minimum);
                }
            }
        }
        break;

    case R4A_ESP32_NVM_PT_FLOAT:
    case R4A_ESP32_NVM_PT_DOUBLE:
        // Convert the string into a numeric value
        valid = (sscanf(valueString, "%llu", &value->u64) == 1);
        if (valid)
        {
            // Validate the value
            value->d = (double)value->i64 / R4A_ESP32_NVM_FLOAT_CONV;
            valid = false;
            switch (parameter->type)
            {
            case R4A_ESP32_NVM_PT_FLOAT:
            case R4A_ESP32_NVM_PT_DOUBLE:
                // Store the value as a double in the value structure
                maximum = ((double)parameter->maximum) / R4A_ESP32_NVM_FLOAT_CONV;
                minimum = ((double)parameter->minimum) / R4A_ESP32_NVM_FLOAT_CONV;
                goodMaxValue = (value->d <= maximum);
                goodMinValue = (value->d >= minimum);
                valid = goodMinValue & goodMaxValue;
                if (!valid)
                {
                    if (display && (!goodMaxValue))
                        display->printf("ERROR: Bad maximum value: %f > %d\r\n", value->d, maximum);
                    if (display && (!goodMinValue))
                        display->printf("ERROR: Bad minimum value: %f > %d\r\n", value->d, minimum);
                }
                break;
           }
        }
        break;

    case R4A_ESP32_NVM_PT_NULLPTR:
        newValue = nullptr;
        valid = true;
        break;

    case R4A_ESP32_NVM_PT_P_CHAR:
        length = strlen(valueString) + 1;
        newValue = (char *)r4aMalloc(length, "NVM char value (newValue)");
        if (!newValue && display && (display != &Serial))
            display->println("ERROR: Failed to allocate parameter value string!");
        else
        {
            strcpy(newValue, valueString);
            value->pcc = newValue;
            valid = true;
        }
        break;
    }

    // Return the valid status
    return valid;
}

//*********************************************************************
// Set the parameter value
// Inputs:
//   parameter: Address of the specified parameter in the table
//   data: New parameter value represented as a 64-bit value
//         binary for values, address for strings
void r4aEsp32NvmSetParameterValue(const R4A_ESP32_NVM_PARAMETER * parameter,
                                  uint64_t data)
{
    void * dataAddress;
    R4A_ESP32_NVM_VALUE value;

    // Display the call
    log_v("r4aEsp32NvmSetParameterValue(%p, %lld(0x%016llx))", (void *)parameter, data, data);

    // Determine the parameter type
    value.u64 = data;
    switch (parameter->type)
    {
    default:
        Serial.printf("ERROR: Invalid parameter type: %d\r\n", parameter->type);
        r4aReportFatalError("nvmSetParameterValue: Invalid parameter type!");
        break;

    case R4A_ESP32_NVM_PT_BOOL:
        *(bool *)(parameter->addr) = (bool)value.u64;
        break;

    case R4A_ESP32_NVM_PT_INT8:
        *(int8_t *)(parameter->addr) = (int8_t)(value.i64);
        break;

    case R4A_ESP32_NVM_PT_UINT8:
        *(uint8_t *)(parameter->addr) = (uint8_t)value.u64;
        break;

    case R4A_ESP32_NVM_PT_INT16:
        *(int16_t *)(parameter->addr) = (int16_t)value.i64;
        break;

    case R4A_ESP32_NVM_PT_UINT16:
        *(uint16_t *)(parameter->addr) = (uint16_t)value.u64;
        break;

    case R4A_ESP32_NVM_PT_INT32:
        *(int32_t *)(parameter->addr) = (int32_t)value.i64;
        break;

    case R4A_ESP32_NVM_PT_UINT32:
        *(uint32_t *)(parameter->addr) = (uint32_t)value.u64;
        break;

    case R4A_ESP32_NVM_PT_INT64:
    case R4A_ESP32_NVM_PT_UINT64:
        *(int64_t *)(parameter->addr) = value.i64;
        break;

    case R4A_ESP32_NVM_PT_FLOAT:
        // Float is represented as a double value in the value structure
        *(float *)(parameter->addr) = (float)value.d;
        break;

    case R4A_ESP32_NVM_PT_DOUBLE:
        // Double is represented as a double value in the value structure
        *(double *)(parameter->addr) = value.d;
        break;

    case R4A_ESP32_NVM_PT_NULLPTR:
        // Free the previously allocated value
        dataAddress = *(char **)(parameter->addr);
        if (dataAddress && (r4aEsp32s3IsAddressInRAM(dataAddress)))
            r4aFree((void *)dataAddress, "NVM char value (newValue)");
        *(const char **)(parameter->addr) = nullptr;
        break;

    case R4A_ESP32_NVM_PT_P_CHAR:
        // Free the previously allocated value
        dataAddress = *(char **)(parameter->addr);
        if (dataAddress && (r4aEsp32s3IsAddressInRAM(dataAddress)))
            r4aFree((void *)dataAddress, "NVM char value (newValue)");
        *(const char **)(parameter->addr) = value.pcc;
        break;
    }
}

//*********************************************************************
// Write a string and zero termination to the parameter file
bool r4aEsp32NvmWriteFileString(File &file, const char * string)
{
    size_t length;

    length = strlen(string) + 1;
    return r4aEsp32NvmWriteFileString(file, string, length);
}

//*********************************************************************
// Write a string to the parameter file
bool r4aEsp32NvmWriteFileString(File &file, const char * string, size_t length)
{
    size_t bytesWritten;

    bytesWritten = file.write((const uint8_t *)string, length);
    return (bytesWritten == length);
}

//*********************************************************************
// Write the parameter value
bool r4aEsp32NvmWriteParameterValue(File &parameterFile,
                                    const R4A_ESP32_NVM_PARAMETER * parameter,
                                    Print * display,
                                    bool debug)
{
    const char * data;
    uint8_t type;
    char typeString[3 + 1];
    char valueString[32];
    bool valid;
    R4A_ESP32_NVM_VALUE value;

    // Display the call
    log_v("r4aEsp32NvmWriteParameterValue(%p, %p, %p, %d)", (void *)&parameterFile, (void *)parameter, (void *)display, debug);

    // Determine the parameter type
    type = parameter->type;
    value.u64 = 0;
    data = nullptr;
    switch (type)
    {
    default:
        if (display)
            display->printf("ERROR: Invalid parameter type: %d\r\n", parameter->type);
        r4aReportFatalError("r4aEsp32NvmWriteParameterValue: Invalid parameter type!");
        break;

    case R4A_ESP32_NVM_PT_BOOL:
        if (display && debug)
            display->printf("%s: %d\r\n", parameter->name, *(bool *)(parameter->addr));
        value.b = *(bool *)(parameter->addr);
        sprintf(valueString, "0x%016llx", value.u64);
        data = valueString;
        break;

    case R4A_ESP32_NVM_PT_INT8:
        if (display && debug)
            display->printf("%s: %d\r\n", parameter->name, *(int8_t *)(parameter->addr));
        value.i64 = (int64_t)(*(int8_t *)(parameter->addr));
        sprintf(valueString, "0x%016llx", value.u64);
        data = valueString;
        break;

    case R4A_ESP32_NVM_PT_UINT8:
        if (display && debug)
            display->printf("%s: %d\r\n", parameter->name, *(uint8_t *)(parameter->addr));
        value.u64 = (uint64_t)(*(uint8_t *)(parameter->addr));
        sprintf(valueString, "0x%016llx", value.u64);
        data = valueString;
        break;

    case R4A_ESP32_NVM_PT_INT16:
        if (display && debug)
            display->printf("%s: %d\r\n", parameter->name, *(int16_t *)(parameter->addr));
        value.i64 = (int64_t)(*(int16_t *)(parameter->addr));
        sprintf(valueString, "0x%016llx", value.u64);
        data = valueString;
        break;

    case R4A_ESP32_NVM_PT_UINT16:
        if (display && debug)
            display->printf("%s: %d\r\n", parameter->name, *(uint16_t *)(parameter->addr));
        value.u16 = (uint64_t)(*(uint16_t *)(parameter->addr));
        sprintf(valueString, "0x%016llx", value.u64);
        data = valueString;
        break;

    case R4A_ESP32_NVM_PT_INT32:
        if (display && debug)
            display->printf("%s: %ld\r\n", parameter->name, *(int32_t *)(parameter->addr));
        value.i64 = (int64_t)(*(int32_t *)(parameter->addr));
        sprintf(valueString, "0x%016llx", value.u64);
        data = valueString;
        break;

    case R4A_ESP32_NVM_PT_UINT32:
        if (display && debug)
            display->printf("%s: %ld\r\n", parameter->name, *(uint32_t *)(parameter->addr));
        value.u32 = (uint64_t)(*(uint32_t *)(parameter->addr));
        sprintf(valueString, "0x%016llx", value.u64);
        data = valueString;
        break;

    case R4A_ESP32_NVM_PT_UINT64:
        if (display && debug)
            display->printf("%s: %lld\r\n", parameter->name, *(uint64_t *)(parameter->addr));
        value.u64 = *(uint64_t *)(parameter->addr);
        sprintf(valueString, "0x%016llx", value.u64);
        data = valueString;
        break;

    case R4A_ESP32_NVM_PT_INT64:
        if (display && debug)
            display->printf("%s: %lld\r\n", parameter->name, *(int64_t *)(parameter->addr));
        value.u64 = *(int64_t *)(parameter->addr);
        sprintf(valueString, "0x%016llx", value.u64);
        data = valueString;
        break;

    case R4A_ESP32_NVM_PT_FLOAT:
        if (display && debug)
            display->printf("%s: %f\r\n", parameter->name, *(float *)(parameter->addr));
        value.d = ((double)(*(float *)(parameter->addr))) * R4A_ESP32_NVM_FLOAT_CONV;
        sprintf(valueString, "0x%016llx", value.u64);
        data = valueString;
        break;

    case R4A_ESP32_NVM_PT_DOUBLE:
        if (display && debug)
            display->printf("%s: %f\r\n", parameter->name, *(double *)(parameter->addr));
        value.d = (*(double *)(parameter->addr)) * R4A_ESP32_NVM_FLOAT_CONV;
        sprintf(valueString, "0x%016llx", value.u64);
        data = valueString;
        break;

    case R4A_ESP32_NVM_PT_NULLPTR:
        if (display && debug)
            display->printf("%s: %p\r\n", parameter->name, *(const char **)(parameter->addr));
        data = "nullptr";
        type = R4A_ESP32_NVM_PT_NULLPTR;
        break;

    case R4A_ESP32_NVM_PT_P_CHAR:
        value.pcc = *(const char **)(parameter->addr);
        if (display && debug)
            display->printf("%s: %p%s%s\r\n", parameter->name,
                            value.pcc, value.pcc ? ": " : "",
                            value.pcc ? value.pcc : "");
        if (value.pcc)
            data = value.pcc;
        else
        {
            data = "nullptr";
            type = R4A_ESP32_NVM_PT_NULLPTR;
        }
        break;
    }

    //  +-------+---+-------+---+-------+---+-------+---+
    //  | Name  | 0 | Type  | 0 | Value | 0 | CR/LF | 0 |
    //  +-------+---+-------+---+-------+---+-------+---+
    //
    // Write the parameter to the file as a triplet of three strings
    // followed by CR and LF
    sprintf(typeString, "%d", type);
    valid = (r4aEsp32NvmWriteFileString(parameterFile, parameter->name)
        && r4aEsp32NvmWriteFileString(parameterFile, typeString)
        && r4aEsp32NvmWriteFileString(parameterFile, data)
        && r4aEsp32NvmWriteFileString(parameterFile, "\r\n"));
    if (display && debug)
        display->printf("%s (type %s): '%s' %s\r\n", parameter->name,
                        r4aEsp32NvmTypeTable[parameter->type],
                        data ? data : "nullptr",
                        valid ? "(valid)" : "(write failed)");
    return valid;
}

//*********************************************************************
// NVM API routines
//*********************************************************************
// Display a parameter
void r4aEsp32NvmDisplayParameter(const R4A_ESP32_NVM_PARAMETER * parameter,
                                 Print * display)
{
    R4A_ESP32_NVM_VALUE value;

    // Display the call
    log_v("r4aEsp32NvmDisplayParameter(%p, %p)", (void *)parameter, (void *)display);

    // Determine the parameter type
    switch (parameter->type)
    {
    default:
        if (display)
            display->printf("ERROR: Invalid parameter type: %d\r\n", parameter->type);
        r4aReportFatalError("nvmDisplayParameter: Invalid parameter type!");
        break;

    case R4A_ESP32_NVM_PT_BOOL:
        value.b = *(bool *)(parameter->addr);
        if (display)
            display->printf("%s: %s\r\n", parameter->name, value.b ? "true" : "false");
        break;

    case R4A_ESP32_NVM_PT_INT8:
        value.i8 = *(int8_t *)(parameter->addr);
        if (display)
            display->printf("%s: %d\r\n", parameter->name, value.i8);
        break;

    case R4A_ESP32_NVM_PT_UINT8:
        value.u8 = *(uint8_t *)(parameter->addr);
        if (display)
            display->printf("%s: %u\r\n", parameter->name, value.u8);
        break;

    case R4A_ESP32_NVM_PT_INT16:
        value.i16 = *(int16_t *)(parameter->addr);
        if (display)
            display->printf("%s: %d\r\n", parameter->name, value.i16);
        break;

    case R4A_ESP32_NVM_PT_UINT16:
        value.u16 = *(uint16_t *)(parameter->addr);
        if (display)
            display->printf("%s: %u\r\n", parameter->name, value.u16);
        break;

    case R4A_ESP32_NVM_PT_INT32:
        value.i32 = *(int32_t *)(parameter->addr);
        if (display)
            display->printf("%s: %ld\r\n", parameter->name, value.i32);
        break;

    case R4A_ESP32_NVM_PT_UINT32:
        value.u32 = *(uint32_t *)(parameter->addr);
        if (display)
            display->printf("%s: %lu\r\n", parameter->name, value.u32);
        break;

    case R4A_ESP32_NVM_PT_INT64:
        value.i64 = *(int64_t *)(parameter->addr);
        if (display)
            display->printf("%s: %lld\r\n", parameter->name, value.i64);
        break;

    case R4A_ESP32_NVM_PT_UINT64:
        value.u64 = *(uint64_t *)(parameter->addr);
        if (display)
            display->printf("%s: %llu\r\n", parameter->name, value.u64);
        break;

    case R4A_ESP32_NVM_PT_FLOAT:
        value.d = (double)(*(float *)(parameter->addr));
        if (display)
            display->printf("%s: %f\r\n", parameter->name, (float)value.d);
        break;

    case R4A_ESP32_NVM_PT_DOUBLE:
        value.d = *(double *)(parameter->addr);
        if (display)
            display->printf("%s: %f\r\n", parameter->name, value.d);
        break;

    case R4A_ESP32_NVM_PT_NULLPTR:
        value.pv = *(void **)(parameter->addr);
        if (display)
        {
            if (value.pv)
                display->printf("%s: %p\r\n", parameter->name, value.pv);
            else
                display->printf("%s: nullptr\r\n", parameter->name);
        }
        break;

    case R4A_ESP32_NVM_PT_P_CHAR:
        value.pcc = *(const char **)(parameter->addr);
        if (display)
        {
            if (value.pcc)
                display->printf("%s: \"%s\"\r\n", parameter->name, value.pcc);
            else
                display->printf("%s: nullptr\r\n", parameter->name);
        }
        break;
    }
}

//*********************************************************************
// Display the parameters
void r4aEsp32NvmDisplayParameters(const R4A_ESP32_NVM_PARAMETER * parameterTable,
                                  int parameterCount,
                                  Print * display)
{
    int i;
    uint16_t * index;
    int j;
    uint16_t temp;

    // Display the call
    log_v("r4aEsp32NvmDisplayParameters(%p, %d, %p)", (void *)parameterTable, parameterCount, (void *)display);

    // Allocate the parameter index table
    index = (uint16_t *)r4aMalloc(parameterCount * sizeof(*index), "Parameter index table (index)");
    if (index)
    {
        // Initialize the index table
        for (i = 0; i < parameterCount; i++)
            index[i] = i;

        // Bubble sort the parameter table
        for (i = 0; i < (parameterCount - 1); i++)
        {
            for (j = i + 1; j < parameterCount; j++)
            {
                if (r4aStricmp(parameterTable[index[i]].name, parameterTable[index[j]].name) > 0)
                {
                    // Switch the entries
                    temp = index[i];
                    index[i] = index[j];
                    index[j] = temp;
                }
            }
        }

        // Display the parameters as listed in the parameter table
        for (i = 0; i < parameterCount; i++)
            r4aEsp32NvmDisplayParameter(&parameterTable[index[i]], display);

        // Done with the index table
        r4aFree((void *)index, "Parameter index table (index)");
    }
    else
        // Display the parameters as listed in the parameter table
        for (int parameter = 0; parameter < parameterCount; parameter++)
            r4aEsp32NvmDisplayParameter(&parameterTable[parameter], display);
}

//*********************************************************************
// Dump the parameter file
void r4aEsp32NvmDumpParameterFile(const char * filePath, Print * display)
{
    size_t bytesRead;
    size_t fileBytes;
    File parameterFile;
    uint8_t * nvmData;

    nvmData = nullptr;
    do
    {
        // Determine if the file exists
        if (!LittleFS.exists(filePath))
        {
            if (display)
                display->printf("Parameter file %s not found!\r\n", filePath);
            break;
        }

        // Open the parameter file
        parameterFile = LittleFS.open(filePath, "r");
        if (!parameterFile)
        {
            if (display)
                 display->printf("ERROR: Failed to open file %s!\r\n", filePath);
            break;
        }

        // Determine the file size
        fileBytes = parameterFile.size();
        if (!fileBytes)
        {
            if (display)
                 display->println("Empty parameter file!");
            break;
        }

        // Allocate the file data buffer
        nvmData = (uint8_t *)r4aMalloc(fileBytes, "NVM file data buffer (nvmData)");
        if (!nvmData)
        {
            if (display && (display != &Serial))
                 display->println("ERROR: Failed to allocate read buffer!");
            break;
        }

        // Read the file into memory
        bytesRead = parameterFile.read(nvmData, fileBytes);
        if (bytesRead != fileBytes)
        {
            if (display)
                 display->println("ERROR: Failed to read file into memory!");
            break;
        }

        r4aDumpBuffer(0, nvmData, bytesRead, display);
    } while (0);

    // Free the NVM data
    if (nvmData)
        r4aFree((void *)nvmData, "NVM file data buffer (nvmData)");

    // Close the file
    if (parameterFile)
        parameterFile.close();
}

//*********************************************************************
// Display the contents of the file
// Inputs:
//   filePath: Name of the file to dump
//   display: Device used for output
void r4aEsp32NvmFileCat(String filePath, Print * display)
{
    int bytesRead;
    size_t bytesToRead;
    uint8_t data[256];
    File file;
    size_t length;
    const char * path;

    // Locate the comment
    path = filePath.c_str();

    // Attempt to open the file
    do
    {
        file = LittleFS.open(path, FILE_READ);
        if (!file)
        {
            if (display)
                display->printf("ERROR: Failed to open file %s!\r\n", path);
            break;
        }

        // Get the file size
        length = file.size();
        do
        {
            // Read the file
            bytesToRead = sizeof(data);
            bytesRead = file.read(data, bytesToRead);
            if(bytesRead < 0)
            {
                display->printf("ERROR: Error reading from file %s\r\n", path);
                break;
            }

            // Display the data
            display->write(data, bytesRead);
            length -= bytesRead;
        } while (length > 0);
    } while (0);

    // Done with the file
    if (file)
        file.close();
}

//*********************************************************************
// Get the default set of parameters
void r4aEsp32NvmGetDefaultParameters(const R4A_ESP32_NVM_PARAMETER * parameterTable,
                                     int parameterCount)
{
    const R4A_ESP32_NVM_PARAMETER * parameter;
    R4A_ESP32_NVM_VALUE value;

    // Display the call
    log_v("r4aEsp32NvmGetDefaultParameters(%p, %d)", (void *)parameterTable, parameterCount);

    // Walk the list of parameters
    for (int index = 0; index < parameterCount; index++)
    {
        // Get the default parameter value
        parameter = &parameterTable[index];
        value.u64 = parameter->value;
        switch (parameter->type)
        {
        // The default float and double parameter values are multiplied
        // by NVM_FLOAT_CONV before being stored in the parameter table.
        // Undo this conversion and restore the float or double value.
        case R4A_ESP32_NVM_PT_FLOAT:
            value.u64 = parameter->value;
            value.d = (double)value.u64 / R4A_ESP32_NVM_FLOAT_CONV;
            // Both float and double values are represented as doubles in
            // the value structure
            break;

        case R4A_ESP32_NVM_PT_DOUBLE:
            value.u64 = parameter->value;
            value.d = (double)value.u64 / R4A_ESP32_NVM_FLOAT_CONV;
            // Both float and double values are represented as doubles in
            // the value structure
            break;
        }
        r4aEsp32NvmSetParameterValue(parameter, value.u64);
    }
}

//*********************************************************************
// Get a set of parameters
// Returns true if successful and false upon failure
bool r4aEsp32NvmGetParameters(const char ** filePath,
                              Print * display,
                              bool debug)
{
    const char * partitionName = "spiffs";
    bool success;

    do
    {
        // Get the default parameters
        r4aEsp32NvmGetDefaultParameters(nvmParameters, nvmParameterCount);

        // Verify that the spiffs partition exists
        if (!r4aEsp32PartitionFind(partitionName))
        {
            r4aEsp32PartitionTableDisplay(&Serial);
            if (display)
                display->printf("ERROR: Add missing %s partition!\r\n", partitionName);
            break;
        }

        // Display the default parameters
        if (debug && display)
        {
            display->printf("\r\nDefault parameters\r\n");
            r4aEsp32NvmDisplayParameters(nvmParameters, nvmParameterCount, display);
        }

        // Start the file system
        log_v("Starting the little file system");
        success = LittleFS.begin(true); // Format LittleFS upon failure
        if (success == false)
        {
            success = LittleFS.begin(false);
            if (success)
                Serial.printf("LittleFS total bytes: %d\r\n", LittleFS.totalBytes());
        }
        if (success == false)
        {
            if (display)
                 display->println("Warning: LittleFS not available, using default parameters");
            else
                 Serial.println("Warning: LittleFS not available, using default parameters");
        }

        // Read the parameters from the file
        else
        {
            log_v("LittleFS total bytes: %d\r\n", LittleFS.totalBytes());
            log_v("LittleFS used bytes: %d\r\n", LittleFS.usedBytes());
            if (r4aEsp32NvmReadParameters(*filePath,
                                          nvmParameters,
                                          nvmParameterCount,
                                          display))
            {
                // Display the loaded parameters
                if (debug && display)
                {
                    display->printf("\r\nUpdated parameters\r\n");
                    r4aEsp32NvmDisplayParameters(nvmParameters, nvmParameterCount, display);
                }
                return true;
            }

            // Display the default parameters
            if (debug && display)
            {
                r4aEsp32NvmDumpParameterFile(*filePath, display);
                display->printf("\r\nCurrent parameters may be corrupt\r\n");
                r4aEsp32NvmDisplayParameters(nvmParameters, nvmParameterCount, display);
            }

            // Since the parameters may be corrupt, set the default values again
            r4aEsp32NvmGetDefaultParameters(nvmParameters, nvmParameterCount);

            // Write the default parameters to the file
            if (display)
                display->printf("WARNING: Overwriting parameter file %s with default parameters!\r\n",
                                *filePath);
            if (!r4aEsp32NvmWriteParameters(*filePath,
                                            nvmParameters,
                                            nvmParameterCount))
            {
                if (display)
                    display->printf("ERROR: Failed to write parameters to file %s!\r\n",
                                    *filePath);
                break;
            }

            // Verify the parameter file
            if (!r4aEsp32NvmReadParameters(*filePath,
                                           nvmParameters,
                                           nvmParameterCount))
            {
                if (display)
                    display->printf("ERROR: Failed to read parameters from %s!\r\n",
                                    *filePath);
                r4aEsp32NvmGetDefaultParameters(nvmParameters, nvmParameterCount);
            }
        }
    } while (0);
    if (display)
    {
        display->println("WARNING: Using default parameters!");

        // Display the default parameters
        if (debug && display)
        {
            display->printf("\r\nDefault parameters\r\n");
            r4aEsp32NvmDisplayParameters(nvmParameters, nvmParameterCount, display);
        }
    }
    return false;
}

//*********************************************************************
// Get a string from the parameter file
bool r4aEsp32NvmGetString(const char ** string, const char ** data, size_t * fileBytes)
{
    bool stringValid;
    char value;

    // Save the string address
    *string = *data;

    // Verify that the file contains a string
    stringValid = false;
    while (*fileBytes)
    {
        // Consume this file byte
        *fileBytes -= 1;
        value = **data;
        *data += 1;

        // Check for end-of-string (zero)
        if (!value)
        {
            stringValid = true;
            break;
        }
    }

    // Return the string found and is zero terminated
    return stringValid;
}

//*********************************************************************
// Display all of the parameters
void r4aEsp32NvmMenuDisplayParameters(const struct _R4A_MENU_ENTRY * menuEntry,
                                      const char * command,
                                      Print * display)
{
    display->println("Parameters");
    r4aEsp32NvmDisplayParameters(nvmParameters,
                                 nvmParameterCount,
                                 display);
    display->println();
}

//*********************************************************************
// Display the partition table
void r4aEsp32NvmMenuDisplayPartitionTable(const struct _R4A_MENU_ENTRY * menuEntry,
                                          const char * command,
                                          Print * display)
{
    r4aEsp32PartitionTableDisplay(display);
}

//*********************************************************************
// Dump the parameter file
void r4aEsp32NvmMenuDumpParameterFile(const struct _R4A_MENU_ENTRY * menuEntry,
                                      const char * command,
                                      Print * display)
{
    r4aEsp32NvmDumpParameterFile(parameterFilePath, display);
}

//*********************************************************************
// Display the contents of the file
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aEsp32NvmMenuFileCat(const R4A_MENU_ENTRY * menuEntry,
                            const char * command,
                            Print * display)
{
    String fileName;
    String filePath;

    // Build the file path
    fileName = r4aMenuGetParameters(menuEntry, command);
    filePath = String("/") + fileName;

    // Display the file contents
    r4aEsp32NvmFileCat(filePath, display);
}

//*********************************************************************
// Copy the file contents to a new file
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aEsp32NvmMenuFileCopy(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display)
{
    int bytesRead;
    size_t bytesToRead;
    int bytesWritten;
    uint8_t data[256];
    File destFile;
    char * destFileName;
    String destFilePath;
    const char * destPath;
    size_t length;
    String parameters;
    File srcFile;
    const char * srcFileName;
    String srcFilePath;
    const char * srcPath;

    do
    {
        // Separate the strings
        parameters = r4aMenuGetParameters(menuEntry, command);
        srcFileName = parameters.c_str();
        destFileName = strstr(srcFileName, " ");
        *destFileName++ = 0;
        destPath = nullptr;

        // Get the source file name
        srcFilePath = String("/") + String(srcFileName);
        srcPath = srcFilePath.c_str();

        // Attempt to open the source file
        length = 0;
        srcFile = LittleFS.open(srcPath, FILE_READ);
        if (!srcFile)
        {
            if (display)
                display->printf("ERROR: Failed to open file %s!\r\n", srcFilePath.c_str());
            break;
        }

        // Get the destination file name
        destFilePath = String("/") + String(destFileName);
        destPath = destFilePath.c_str();

        // Attempt to open the destination file
        destFile = LittleFS.open(destPath, FILE_WRITE);
        if (!destFile)
        {
            if (display)
                display->printf("ERROR: Failed to open file %s!\r\n", destFilePath.c_str());
            break;
        }

        // Get the file size
        length = srcFile.size();
        do
        {
            // Read the source file
            bytesToRead = sizeof(data);
            bytesRead = srcFile.read(data, bytesToRead);
            if(bytesRead < 0)
            {
                display->printf("ERROR: Error reading from file %s\r\n", srcFilePath.c_str());
                break;
            }

            // Write the data to the destination file
            bytesWritten = destFile.write(data, bytesRead);
            if(bytesWritten < bytesRead)
            {
                display->printf("ERROR: Error writing to file %s\r\n", destFilePath.c_str());
                break;
            }

            // Account for the data read
            length -= bytesRead;
        } while (length > 0);
    } while (0);

    // Done with the files
    if (destFile)
        destFile.close();
    if (srcFile)
        srcFile.close();

    // Delete the destination file if necessary
    if (length && destPath)
        if (!LittleFS.remove(destPath))
            display->printf("ERROR: Failed to delete file %s!r\r\n", destPath);
}

//*********************************************************************
// Dump the file contents
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aEsp32NvmMenuFileDump(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display)
{
    int bytesRead;
    size_t bytesToRead;
    uint8_t data[256];
    File file;
    String fileName;
    String filePath;
    size_t length;
    size_t offset;
    const char * path;

    do
    {
        // Get the file name
        fileName = r4aMenuGetParameters(menuEntry, command);
        filePath = String("/") + fileName;
        path = filePath.c_str();

        // Attempt to open the file
        file = LittleFS.open(path, FILE_READ);
        if (!file)
        {
            if (display)
                display->printf("ERROR: Failed to open file %s!\r\n", filePath.c_str());
            break;
        }

        // Get the file size
        length = file.size();
        offset = 0;
        do
        {
            // Read the file
            bytesToRead = sizeof(data);
            bytesRead = file.read(data, bytesToRead);
            if(bytesRead < 0)
            {
                display->printf("ERROR: Error reading from file %s\r\n", filePath.c_str());
                break;
            }

            // Display the data
            r4aDumpBuffer(offset, data, bytesRead, &Serial);
            length -= bytesRead;
            offset += bytesRead;
        } while (length > 0);
    } while (0);

    // Done with the file
    if (file)
        file.close();
}

//*********************************************************************
// List the contents of the current directory
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aEsp32NvmMenuFileList(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display)
{
    File rootDir;
    bool directory;
    File file;
    bool headerDisplayed;
    const char * name;
    size_t size;

    // Start at the beginning of the directory
    rootDir = LittleFS.open("/", FILE_READ);
    if (rootDir)
    {
        // List the contents of the directory
        headerDisplayed = false;
        while (1)
        {
            // Open the next file
            file = rootDir.openNextFile();
            if (!file)
            {
                if (headerDisplayed == false)
                    display->printf("No files found\r\n");
                break;
            }

            // Get the file attributes
            name = file.name();
            size = file.size();
            directory = file.isDirectory();

            // Done with this file
            file.close();

            // Display the attributes
            if (headerDisplayed == false)
            {
                headerDisplayed = true;
                display->printf("      Size   Dir   File Name\r\n");
                //                        1                  1         2
                //               1234567890   123   12345678901234567890
                display->printf("----------   ---   --------------------\r\n");
            }

            // Display the file attributes
            display->printf("%10d   %s   %s\r\n", size, directory ? "Dir" : "   ", name);
        }

        // Close the directory
        close(rootDir);
    }
}

//*********************************************************************
// Move (rename) a file
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aEsp32NvmMenuFileMove(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display)
{
    File destFile;
    char * destFileName;
    String destFilePath;
    const char * destPath;
    String parameters;
    File srcFile;
    const char * srcFileName;
    String srcFilePath;
    const char * srcPath;

    do
    {
        // Separate the strings
        parameters = r4aMenuGetParameters(menuEntry, command);
        srcFileName = parameters.c_str();
        destFileName = strstr(srcFileName, " ");
        *destFileName++ = 0;

        // Get the source file name
        srcFilePath = String("/") + String(srcFileName);
        srcPath = srcFilePath.c_str();

        // Get the destination file name
        destFilePath = String("/") + String(destFileName);
        destPath = destFilePath.c_str();

        // Rename the file
        if (LittleFS.rename(srcPath, destPath) == false)
        {
            if (display)
                display->printf("ERROR: Failed to rename file %s!\r\n", srcFilePath.c_str());
            break;
        }
    } while (0);
}

//*********************************************************************
// Remove the file
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aEsp32NvmMenuFileRemove(const R4A_MENU_ENTRY * menuEntry,
                               const char * command,
                               Print * display)
{
    static String fileName;
    String filePath;
    const char * name;
    bool removed;

    // Get the file name
    fileName = r4aMenuGetParameters(menuEntry, command);
    filePath = String("/") + fileName;
    name = filePath.c_str();

    // Delete the file
    removed = LittleFS.remove(name);
    if (display)
    {
        // Display the results
        if (removed)
            display->printf("File %s removed\r\n", name);
        else
            display->printf("File %s not found\r\n", name);
    }
}

//*********************************************************************
// Get default parameters
void r4aEsp32NvmMenuGetDefaultParameters(const struct _R4A_MENU_ENTRY * menuEntry,
                                         const char * command,
                                         Print * display)
{
    r4aEsp32NvmGetDefaultParameters(nvmParameters, nvmParameterCount);
}

//*********************************************************************
// Display the help text with PPP
[[deprecated("Use r4aMenuHelpSuffix instead.")]]
void r4aEsp32NvmMenuHelpPppp(const struct _R4A_MENU_ENTRY * menuEntry,
                             const char * align,
                             Print * display)
{
    display->printf("%s pppp: %s%s\r\n",
                    menuEntry->command, align, menuEntry->helpText);
}

//*********************************************************************
// Display the help text with PPPP XXXX
[[deprecated("Use r4aMenuHelpSuffix instead.")]]
void r4aEsp32NvmMenuHelpPpppXxxx(const struct _R4A_MENU_ENTRY * menuEntry,
                                 const char * align,
                                 Print * display)
{
    display->printf("%s pppp xxxx: %s%s\r\n",
                    menuEntry->command, align, menuEntry->helpText);
}

//*********************************************************************
// Download a file from a web server
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aEsp32NvmMenuHttpFileGet(const R4A_MENU_ENTRY * menuEntry,
                                const char * command,
                                Print * display)
{
    int bytesWritten;
    File file;
    const char * fileName;
    HTTPClient http;
    int httpStatus;
    size_t length;
    const char * path;
    String payload;
    const char * url;
    String urlString;

    do
    {
        // Get the URL
        length = 0;
        urlString = r4aMenuGetParameters(menuEntry, command);
        url = urlString.c_str();

        // Get the web page
        http.begin(url);
        httpStatus = http.GET();
        if (httpStatus <= 0)
        {
            if (display)
                display->printf("ERROR: HTTP GET failed, error: %s\r\n",
                                http.errorToString(httpStatus).c_str());
            break;
        }

        // Get the file name
        path = url;
        while (1)
        {
            fileName = strstr(path, "/");
            if (fileName == nullptr)
                break;
            path = &fileName[1];
        }

        // Build the file path
        if (strlen(path))
            path -= 1;
        else
            path = "/index.html";


        // Attempt to open the destination file
        file = LittleFS.open(path, FILE_WRITE);
        if (!file)
        {
            if (display)
                display->printf("ERROR: Failed to open file %s!\r\n", path);
            break;
        }

        // Determine the response length
        payload = http.getString();
        length = payload.length();

        // Write the data to the destination file
        bytesWritten = file.write((uint8_t *)payload.c_str(), length);
        length -= bytesWritten;
        if(length > 0)
            display->printf("ERROR: Error writing to file %s\r\n", path);
    } while (0);

    // Done with the files
    if (file)
        file.close();

    // Delete the file if necessary
    if (length)
        LittleFS.remove(path);

    // Done with the HTTP client
    http.end();
}

//*********************************************************************
// Clear the parameter
void r4aEsp32NvmMenuParameterClear(const struct _R4A_MENU_ENTRY * menuEntry,
                                   const char * command,
                                   Print * display)
{
    // Get the parameter name
    String line = r4aMenuGetParameters(menuEntry, command);
    const char * name = line.c_str();

    // Clear the parameter
    r4aEsp32NvmParameterClear(parameterFilePath,
                              nvmParameters,
                              nvmParameterCount,
                              name,
                              display);
}

//*********************************************************************
// Display the parameter
void r4aEsp32NvmMenuParameterDisplay(const struct _R4A_MENU_ENTRY * menuEntry,
                                     const char * command,
                                     Print * display)
{
    const R4A_ESP32_NVM_PARAMETER * parameter;

    // Get the parameter name
    String line = r4aMenuGetParameters(menuEntry, command);
    const char * name = line.c_str();

    // Display the parameter
    parameter = r4aEsp32NvmParameterLookup(nvmParameters,
                                           nvmParameterCount,
                                           name,
                                           display);
    if (!parameter)
        // Unknown parameter
        display->printf("WARNING: %s is not a parameter!\r\n", name);
    else
        r4aEsp32NvmDisplayParameter(parameter, display);
}

//*********************************************************************
// Read the parameters from the parameter file
void r4aEsp32NvmMenuParameterFileRead(const struct _R4A_MENU_ENTRY * menuEntry,
                                      const char * command,
                                      Print * display)
{
    r4aEsp32NvmReadParameters(parameterFilePath,
                              nvmParameters,
                              nvmParameterCount,
                              display);
}

//*********************************************************************
// Write the parameters to the parameter file
void r4aEsp32NvmMenuParameterFileWrite(const struct _R4A_MENU_ENTRY * menuEntry,
                                       const char * command,
                                       Print * display)
{
    r4aEsp32NvmWriteParameters(parameterFilePath,
                               nvmParameters,
                               nvmParameterCount,
                               display);
}

//*********************************************************************
// Set the parameter value
void r4aEsp32NvmMenuParameterSet(const struct _R4A_MENU_ENTRY * menuEntry,
                                 const char * command,
                                 Print * display)
{
    const R4A_ESP32_NVM_PARAMETER * parameter;
    char * value;

    // Get the parameter name
    String line = r4aMenuGetParameters(menuEntry, command);

    // Copy the remaining portion of the string into a buffer area
    char cmd[line.length() + 1];
    strcpy(cmd, line.c_str());

    // Save the parameter name string
    const char * name = cmd;

    // Skip over the parameter name
    value = cmd;
    while (*value && (*value != ' ') && (*value != '\t'))
        value++;

    // Zero terminate the name and skip over the white space
    while ((*value == ' ') || (*value == '\t'))
        *value++ = 0;

    // Determine if a second parameter was specified
    if (strlen(name) == 0)
        display->println("No parameter specified");
    else
    {
        // Get the parameter structure
        parameter = r4aEsp32NvmParameterLookup(nvmParameters,
                                               nvmParameterCount,
                                               name,
                                               display);
        if (!parameter)
            // Unknown parameter
            display->printf("WARNING: %s is not a parameter!\r\n", name);
        else
            // Set the string value, may be an empty string
            // Use nvmParameterClear to set the value to nullptr
            r4aEsp32NvmParameterSet(parameterFilePath,
                                    nvmParameters,
                                    nvmParameterCount,
                                    parameter,
                                    value,
                                    display);
    }
}

//*********************************************************************
// Clear a parameter by setting its value to zero
void r4aEsp32NvmParameterClear(const char * filePath,
                               const R4A_ESP32_NVM_PARAMETER * parameterTable,
                               int parameterCount,
                               const char * name,
                               Print * display)
{
    const R4A_ESP32_NVM_PARAMETER * parameter;

    // Lookup the parameter
    parameter = r4aEsp32NvmParameterLookup(parameterTable, parameterCount, name, display);
    if (!parameter)
    {
        // Unknown parameter
        if (display)
            display->printf("WARNING: %s is not a parameter!\r\n", name);
    }
    else
    {
        // Clear the parameter
        r4aEsp32NvmSetParameterValue(parameter, 0);

        // Write the parameters to the file
        r4aEsp32NvmWriteParameters(filePath,
                                   parameterTable,
                                   parameterCount,
                                   display);

        // Display the updated parameter value
        r4aEsp32NvmDisplayParameter(parameter, display);
    }
}

//*********************************************************************
// Look up a parameter by address
const R4A_ESP32_NVM_PARAMETER * r4aEsp32NvmParameterLookup(const R4A_ESP32_NVM_PARAMETER * parameterTable,
                                                           int parameterCount,
                                                           void * address,
                                                           Print * display)
{
    int index;
    const R4A_ESP32_NVM_PARAMETER * parameter;

    // Display the call
    log_v("r4aEsp32NvmParameterLookup(%p, %d, %p, %p)", (void *)parameterTable, parameterCount, address, (void *)display);

    // Look up the parameter
    parameter = nullptr;
    for (index = 0; index < parameterCount; index++)
    {
        if (parameterTable[index].addr == address)
        {
            parameter = &parameterTable[index];
            break;
        }
    }
    return parameter;
}

//*********************************************************************
// Look up a parameter by name
const R4A_ESP32_NVM_PARAMETER * r4aEsp32NvmParameterLookup(const R4A_ESP32_NVM_PARAMETER * parameterTable,
                                                           int parameterCount,
                                                           const char * name,
                                                           Print * display)
{
    int index;
    const R4A_ESP32_NVM_PARAMETER * parameter;

    // Display the call
    log_v("r4aEsp32NvmParameterLookup(%p %d, %p, %p)", (void *)parameterTable, parameterCount, (const void *)name, (void *)display);

    // Look up the parameter
    parameter = nullptr;
    for (index = 0; index < parameterCount; index++)
    {
        if (r4aStricmp(parameterTable[index].name, name) == 0)
        {
            parameter = &parameterTable[index];
            break;
        }
    }
    return parameter;
}

//*********************************************************************
// Set a parameter value
bool r4aEsp32NvmParameterSet(const char * filePath,
                             const R4A_ESP32_NVM_PARAMETER * parameterTable,
                             int parameterCount,
                             const R4A_ESP32_NVM_PARAMETER * parameter,
                             const char * valueString,
                             Print * display,
                             bool debug)
{
    R4A_ESP32_NVM_VALUE value;

    // Convert the valueString into a number or an allocated string
    if (r4aEsp32NvmParseValue(parameter, valueString, &value, display))
    {
        bool success;

        // Successful conversion, set the value
        r4aEsp32NvmSetParameterValue(parameter, value.u64);

        // Write the parameters to the file
        success = r4aEsp32NvmWriteParameters(filePath,
                                             parameterTable,
                                             parameterCount,
                                             display,
                                             debug);

        // Display the updated parameter value
        if (success)
            r4aEsp32NvmDisplayParameter(parameter, display);
        return success;
    }
    return false;
}

//*********************************************************************
// Parse the parameter file data
bool r4aEsp32NvmParseParameters(const R4A_ESP32_NVM_PARAMETER * parameterTable,
                                int parameterCount,
                                const char * nvmData,
                                size_t fileBytes,
                                uint8_t * availableParameters,
                                Print * display)
{
    const char * crLfString;
    const char * data;
    size_t delta;
    int index;
    const char * name;
    const R4A_ESP32_NVM_PARAMETER * parameter;
    const char * typeString;
    int type;
    bool validParameters;
    R4A_ESP32_NVM_VALUE value;
    const char * valueString;

    // Walk through the file data
    data = nvmData;
    validParameters = false;
    while (fileBytes)
    {
        //  +-------+---+-------+---+-------+---+-------+---+
        //  | Name  | 0 | Type  | 0 | Value | 0 | CR/LF | 0 |
        //  +-------+---+-------+---+-------+---+-------+---+
        //
        // Read the parameter triplet of strings followed by CR and LF
        // from the file
        validParameters = true;
        if (!(r4aEsp32NvmGetString(&name, &data, &fileBytes)
              && r4aEsp32NvmGetString(&typeString, &data, &fileBytes)
              && r4aEsp32NvmGetString(&valueString, &data, &fileBytes)
              && r4aEsp32NvmGetString(&crLfString, &data, &fileBytes)
              && (crLfString[0] == '\r')
              && (crLfString[1] == '\n')
              && (crLfString[2] == 0)))
        {
            delta = data - nvmData;
            if (display)
                display->printf("ERROR: String at offset %d (0x%x) is invalid!\r\n",
                                delta, delta);
            validParameters = false;
            break;
        }

        // The type string should be a numeric value
        if (sscanf(typeString, "%d", &type) != 1)
        {
            delta = typeString - nvmData;
            if (display)
                display->printf("ERROR: Type string not a number at offset %d (0x%x)!\r\n",
                                delta, delta);
            validParameters = false;
            break;
        }

        // Look up the parameter
        parameter = r4aEsp32NvmParameterLookup(parameterTable, parameterCount, name, display);
        if (!parameter)
        {
            // Unknown parameter, skip this triplet
            if (display)
                display->printf("WARNING: %s is not a parameter!\r\n", name);
            continue;
        }

        // Parse the value
        value.u64 = 0;
        if (type != R4A_ESP32_NVM_PT_NULLPTR)
        {
            if (!r4aEsp32NvmParseValue(parameter, valueString, &value, display))
            {
                delta = valueString - nvmData;
                if (display)
                    display->printf("ERROR: Invalid %s value string at offset %d (0x%x)!\r\n",
                                    name, delta, delta);
                validParameters = false;
                break;
            }
        }

        // Set the value
        r4aEsp32NvmSetParameterValue(parameter, value.u64);
        index = parameter - parameterTable;
        availableParameters[index >> 3] |= 1 << (index & 7);
    }

    // Return the file read status
    if (fileBytes)
        validParameters = false;
    return validParameters;
}

//*********************************************************************
// Read a line from the file
// Inputs:
//   file: Address of the File object
//   line: Address of the buffer to receive the zero terminated line
//   lineLength: Number of bytes in the buffer
//   display: Device used for output, passed to computeWayPoint
// Outputs:
//   Returns true if the line was successfully read
bool r4aEsp32NvmReadLine(File * file,
                uint8_t * line,
                size_t lineLength,
                Print * display)
{
    size_t bytesAvailable;
    uint8_t data;
    uint8_t * lineEnd;

    // Walk the data in the file until the buffer is full
    lineEnd = &line[lineLength - 1];
    while (line < lineEnd)
    {
        // Determine if there is more data in the file
        bytesAvailable = file->available();
        if (bytesAvailable == 0)
            break;

        // Read the next character from the file
        data = file->read();

        // Check for the end of the line
        if (data)
        {
            *line++ = data;
            if (data == '\n')
            {
                // Zero terminate the string of characters
                *line = 0;
                return true;
            }
        }
    }

    // Failed to find the end of the line
    return false;
}

//*********************************************************************
// Read the parameters from a file
bool r4aEsp32NvmReadParameters(const char * filePath,
                               const R4A_ESP32_NVM_PARAMETER * parameterTable,
                               int parameterCount,
                               Print * display)
{
    size_t bytesRead;
    size_t fileBytes;
    File parameterFile;
    uint8_t * nvmData;
    bool status;

    // Display the call
    log_v("r4aEsp32NvmReadParameters(%p, %p %d, %p)", (void *) filePath, (void *)parameterTable, parameterCount, (void *)display);

    // Allocate the available parameter bitmap
    int parameterBytes = (parameterCount + 7) >> 3;
    uint8_t availableParameters[parameterBytes];
    memset(availableParameters, 0, parameterBytes);

    nvmData = nullptr;
    status = false;
    do
    {
        // Determine if the file exists
        if (!LittleFS.exists(filePath))
        {
            if (display)
                display->printf("Parameter file %s not found!\r\n", filePath);
            break;
        }

        // Open the parameter file
        parameterFile = LittleFS.open(filePath, "r");
        if (!parameterFile)
        {
            if (display)
                display->printf("ERROR: Failed to open file %s!\r\n", filePath);
            break;
        }

        // Determine the file size
        fileBytes = parameterFile.size();
        if (!fileBytes)
            break;

        // Allocate the file data buffer
        nvmData = (uint8_t *)r4aMalloc(fileBytes, "NVM file data buffer (nvmData)");
        if (!nvmData)
        {
            if (display && (display != &Serial))
                display->println("ERROR: Failed to allocate read buffer!");
            break;
        }

        // Read the file into memory
        bytesRead = parameterFile.read(nvmData, fileBytes);
        if (bytesRead != fileBytes)
        {
            if (display)
                display->println("ERROR: Failed to read file into memory!");
            break;
        }

        if (display)
            display->printf("Loading parameters from %s\r\n", filePath);

        // Parse the parameters
        if (!r4aEsp32NvmParseParameters(parameterTable,
                                        parameterCount,
                                        (const char *)nvmData,
                                        fileBytes,
                                        availableParameters,
                                        display))
            break;

        // Display any missing parameters
        for (int index = 0; index < parameterCount; index++)
            if (parameterTable[index].required
                && (!(availableParameters[index >> 3] & (1 << (index & 7)))))
            {
                if (display)
                    display->printf("Parameter %s is using the default value\r\n", parameterTable[index].name);
            }
        status = true;
    } while (0);

    // Free the NVM data
    if (nvmData)
        r4aFree((void *)nvmData, "NVM file data buffer (nvmData)");

    // Close the file
    if (parameterFile)
        parameterFile.close();

    // Return status indicating if the parameters were successfully read
    return status;
}

//*********************************************************************
// Write the parameters to a file
bool r4aEsp32NvmWriteParameters(const char * filePath,
                                const R4A_ESP32_NVM_PARAMETER * parameterTable,
                                int parameterCount,
                                Print * display,
                                bool debug)
{
    bool success;

    // Display the call
    log_v("r4aEsp32NvmWriteParameters(%p, %p %d, %p, %d)", (void *) filePath, (void *)parameterTable, parameterCount, (void *)display, debug);

    // Attempt to open the file
    success = false;
    log_v("Opening the parameter file %s for write", (void *) filePath);
    File parameterFile = LittleFS.open(filePath, "w");
    if (!parameterFile)
    {
        if (display)
            display->printf("ERROR: Failed to create file %s!\r\n", filePath);
        else
            Serial.printf("ERROR: Failed to create file %s!\r\n", filePath);
    }
    else
    {
        log_v("Parameter file %s opened successfully", (void *) filePath);
        if (display)
            display->printf("Saving parameters to %s\r\n", filePath);

        // Walk the list of parameters
        success = true;
        for (int parameter = 0; parameter < parameterCount; parameter++)
        {
            // Save the parameter value
            success = r4aEsp32NvmWriteParameterValue(parameterFile,
                                                     &parameterTable[parameter],
                                                     display,
                                                     debug);
            if (!success)
            {
                if (display)
                    display->printf("ERROR: Failed to write parameter %s to file %s!\r\n",
                                    parameterTable[parameter].name, filePath);
            }
        }

        // Done with the file
        parameterFile.close();
    }
    return success;
}

//*********************************************************************
// Reset the system
void r4aEsp32MenuSystemReset(const struct _R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display)
{
    r4aEsp32SystemReset();
}

//*********************************************************************
// Find the specified partition
bool r4aEsp32PartitionFind(const char * name)
{
    // Initialize the partition list iterator
    esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_ANY,
                                                     ESP_PARTITION_SUBTYPE_ANY,
                                                     NULL);

    // Determine if any partitions exist in the list
    if (pi != NULL)
    {
        // Walk through the list of partitions
        do
        {
            // Get the partition data
            const esp_partition_t *p = esp_partition_get(pi);

            // Determine if the requested partition was found
            if (strcmp(p->label, name) == 0)
                // The partition was found
                return true;

            // Get the next partition in the list
        } while ((pi = (esp_partition_next(pi))));
    }

    // The partition does not exist
    return false;
}

//*********************************************************************
// Display the partition table
void r4aEsp32PartitionTableDisplay(Print * display)
{
    // Display the table header
    display->println(".-----------------------------------------------------.");
    display->println("|                ESP32 Partition table                |");
    display->println("|-----------------------------------------------------|");
    display->println("| Type | Sub |  Offset  |   Size   |       Label      |");
    display->println("| ---- | --- | -------- | -------- | ---------------- |");

    // Initialize the partition list iterator
    esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);

    // Determine if any partitions exist in the list
    if (pi != NULL)
    {
        // Walk through the list of partitions
        do
        {
            // Get the partition data
            const esp_partition_t *p = esp_partition_get(pi);

            // Display the partition data
            display->printf("|  %02x  | %02x  | 0x%06lx | 0x%06lx | %-16s |\r\n",
                            p->type,
                            p->subtype,
                            p->address,
                            p->size,
                            p->label);

            // Get the next partition in the list
        } while ((pi = (esp_partition_next(pi))));
    }

    // Display the table footer
    display->println("'-----------------------------------------------------'");
}

//*********************************************************************
// System reset
void r4aEsp32SystemReset()
{
    ESP.restart();
    while (1);
}

//*********************************************************************
// Determine if the address is SRAM that supports DMA
bool r4aEsp32s3IsAddressInDMARAM(void * addr)
{
    return r4aEsp32s3IsAddressInSRAM1(addr) || r4aEsp32s3IsAddressInSRAM2(addr);
}

//*********************************************************************
// Determine if the address is in the EEPROM (Flash)
bool r4aEsp32s3IsAddressInEEPROM(void * addr)
{
    return ((addr >= (void *)0x3f400000) && (addr <= (void *)0x3f7fffff));
}

//*********************************************************************
// Determine if the address is in PSRAM (SPI RAM)
bool r4aEsp32s3IsAddressInPSRAM(void * addr)
{
    return (((addr >= (void *)0x37000000) && (addr <= (void *)0x3bffffff))
        || ((addr >= (void *)0x44000000) && (addr <= (void *)0x47ffffff)));
}

//*********************************************************************
// Determine if the address is in PSRAM, SRAM or RTC fast memory
bool r4aEsp32s3IsAddressInRAM(void * addr)
{
    return r4aEsp32s3IsAddressInSRAM(addr) || r4aEsp32s3IsAddressInPSRAM(addr)
        || r4aEsp32s3IsAddressInRtcFastMemory(addr);
}

//*********************************************************************
bool r4aEsp32s3IsAddressInROM(void * addr)
{
    return (((addr >= (void *)0x3ff00000) && (addr <= (void *)0x3ff0ffff))
        ||  ((addr >= (void *)0x40000000) && (addr <= (void *)0x4005ffff)));
}

//*********************************************************************
// Determine if the address is in RTC fast memory
bool r4aEsp32s3IsAddressInRtcFastMemory(void * addr)
{
    // Instruction bus addresses, PRO_CPU only, see pages 28-29 in the TRM
    return (xPortGetCoreID() == 0)
        && (((addr >= (void *)0x40070000) && (addr <= (void *)0x40071fff))
            || ((addr >= (void *)0x600fc0000) && (addr <= (void *)0x600fdfff)));
}

//*********************************************************************
// Determine if the address is in RTC slow memory
bool r4aEsp32s3IsAddressInRtcSlowMemory(void * addr)
{
    // Instruction bus addresses, PRO_CPU only, see pages 28-29 in the TRM
    return (xPortGetCoreID() == 0)
        && ((addr >= (void *)0x50000000) && (addr <= (void *)0x50001fff));
}

//*********************************************************************
// Determine if the address is in SRAM
bool r4aEsp32s3IsAddressInSRAM(void * addr)
{
    return r4aEsp32s3IsAddressInSRAM0(addr) || r4aEsp32s3IsAddressInSRAM1(addr)
        || r4aEsp32s3IsAddressInSRAM2(addr);
}

//*********************************************************************
// Determine if the address is in SRAM0
bool r4aEsp32s3IsAddressInSRAM0(void * addr)
{
    // Instruction bus addresses
    return ((addr >= (void *)0x40370000) && (addr <= (void *)0x4039ffff));
}

//*********************************************************************
// Determine if the address is in SRAM1
bool r4aEsp32s3IsAddressInSRAM1(void * addr)
{
    // Data bus addresses
    return ((addr >= (void *)0x3fc80000) && (addr <= (void *)0x3fc9ffff))
        // Instruction bus addresses
        || ((addr >= (void *)0x403a0000) && (addr <= (void *)0x403bffff));
}

//*********************************************************************
// Determine if the address is in SRAM2
bool r4aEsp32s3IsAddressInSRAM2(void * addr)
{
    return ((addr >= (void *)0x3fcae000) && (addr <= (void *)0x3fcdffff));
}

//*********************************************************************
// Determine if the address is a peripheral register
bool r4aEsp32s3IsAddressPeripheralRegister(void * addr)
{
    return ((addr >= (void *)0x60000000) && (addr <= (void *)0x600fbfff));
}
