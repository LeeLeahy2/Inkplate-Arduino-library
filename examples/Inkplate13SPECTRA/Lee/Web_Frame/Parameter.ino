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
    float fValue;
    int iValue;
    const char * parameterName;
    int parameterType;
    bool success;
    union
    {
        bool * b;
        char * c;
        double * d;
        float * f;
        int * i;
        int8_t * i8;
        int16_t * i16;
        int32_t * i32;
        int64_t * i64;
        uint8_t * u8;
        uint16_t * u16;
        uint32_t * u32;
        uint64_t * u64;
        void * v;
    } variable;

    // Get the numeric parameter value
    success = true;
    parameterName = csvParameterTable[parameter]._parameterName;
    parameterType = csvParameterTable[parameter]._parameterType;
    variable.v = csvParameterTable[parameter]._parameterVariable;
    if ((parameterType != TYPE_STRING)
        && (sscanf(parameterValueString, "0x%x", &iValue) != 1)
        && (sscanf(parameterValueString, "%d", &iValue) != 1))
    {
        success = false;
        sprintf(errorMessage, "ERROR: Invalid value (%s) for parameter %s\r\n",
                parameterValueString, parameterName);
        Serial.printf("%s\r\n", errorMessage);
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
            *variable.b = (bool)iValue;
            break;

        case TYPE_TENTHS:
            fValue = iValue;
            *variable.f = fValue / 10.;
            break;

        case TYPE_UINT16:
            *variable.u16 = (uint16_t)iValue;
            break;
    }
    return success;
}
