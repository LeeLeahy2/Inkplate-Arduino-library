// --- WiFi Configuration ---
const char *ssid = "";
const char *password = "";

// --- User and Location Info ---
String myUsername = "Soldered"; // User's name to be displayed on screen
String myCity = "Osijek";       // City name for weather data
int timeZone =
    1; // timeZone is the number in (UTC + number) in your time zone UTC + 1 for Osijek, UTC - 4 for New York City
float latitude =  45.5550;  // Latitude of the city
float longitude = 18.6955; // Longitude of the city

bool metricUnits = true;  // set this to false if you wish to use Imperial units

const char* ntpServer = "pool.ntp.org";  // in case you want to use a different one
