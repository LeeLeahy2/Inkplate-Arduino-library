/**********************************************************************
  Parameters.h

  Declare the parameters and set default values
**********************************************************************/

#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

//****************************************
// NVM
//****************************************

const char * parameterFilePath;

//****************************************
// Web_Frame
//****************************************

const char * csvFileUrl;

//****************************************
// WiFi Access Points
//****************************************

#define WIFI_SSID_ENTRIES       8
#define WIFI_PASSWORD_ENTRIES   WIFI_SSID_ENTRIES

// Public access point SSID and password
const char * ssid[WIFI_SSID_ENTRIES];
const char * password[WIFI_PASSWORD_ENTRIES];

//****************************************
// NVM Parameters
//****************************************

const R4A_ESP32_NVM_PARAMETER nvmParameters[] =
{
    // NVM parameters
// Required    Type                  Minimum     Maximum        Address                     Name            Default Value
    {true,  R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &parameterFilePath,         "ParamFile",    R4A_ESP32_NVM_STRING("/Parameters.txt")},

    // Web_Frame parameters
    {true, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &csvFileUrl,                 "csvFileUrl",   R4A_ESP32_NVM_STRING("https://Your website/Your path/Your CSV file")},

    // WiFi: Public Access Points (APs)
// Required    Type                  Minimum     Maximum        Address                     Name            Default Value
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &ssid[0],                 "WifiSsid1",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &password[0],             "WifiPass1",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &ssid[1],                 "WifiSsid2",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &password[1],             "WifiPass2",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &ssid[2],                 "WifiSsid3",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &password[2],             "WifiPass3",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &ssid[3],                 "WifiSsid4",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &password[3],             "WifiPass4",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &ssid[4],                 "WifiSsid5",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &password[4],             "WifiPass5",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &ssid[5],                 "WifiSsid6",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &password[5],             "WifiPass6",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &ssid[6],                 "WifiSsid7",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &password[6],             "WifiPass7",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &ssid[7],                 "WifiSsid8",    R4A_ESP32_NVM_STRING("")},
    {false, R4A_ESP32_NVM_PT_P_CHAR, 0,          0,             &password[7],             "WifiPass8",    R4A_ESP32_NVM_STRING("")},
};
const int nvmParameterCount = sizeof(nvmParameters) / sizeof(nvmParameters[0]);

#endif  // __PARAMETERS_H__
