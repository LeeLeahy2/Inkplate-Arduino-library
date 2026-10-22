/**
 **************************************************
 * @file        Parameter.ino
 * @brief       Parameter support routines for Web_Frame
 *
 * @details     Extract parameters from the CSV control file
 *
 * @author      Lee Leahy
 * @date        2026-10-22
 **************************************************/

//----------------------------------------
// Set a parameter's value
//----------------------------------------
bool parameterSetValue(int parameter, const char * parameterValueString)
{
    char errorMessage[80];
    const char * parameterName;
    int parameterType;
    bool success;
    int value;

    // Get the numeric parameter value
    success = true;
    parameterName = csvParameterTable[parameter]._parameterName;
    parameterType = csvParameterTable[parameter]._parameterType;
    if ((parameterType != TYPE_STRING)
        && (sscanf(parameterValueString, "0x%x", &value) != 1)
        && (sscanf(parameterValueString, "%d", &value) != 1))
    {
        success = false;
        sprintf(errorMessage, "ERROR: Invalid value (%s) for parameter %s\r\n",
                parameterValueString, parameterName);
        Serial.printf("%s\r\n", errorMessage);
reportFatalError(errorMessage);
    }

    // Process the parameter
    switch (parameterType)
    {
        // Handle unknown types
        default:
            sprintf(errorMessage, "Parameter %s, type not implemented", parameterName);
            Serial.printf("%s\r\n", errorMessage);
            reportFatalError(errorMessage);
            break;

        case TYPE_BOOL:
            *(bool *)csvParameterTable[parameter]._parameterVariable;
            break;
    }
}
