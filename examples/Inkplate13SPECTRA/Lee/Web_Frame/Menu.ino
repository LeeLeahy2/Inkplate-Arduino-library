/**
 **************************************************
 * @file        Menu.ino
 * @brief       Serial menu support routines for Web_Frame
 *
 * @details     Code copied from R4A_ESP32 and R4A_Robot libraries.
 *
 * @author      Lee Leahy
 * @date        2026-11-06
 **************************************************/

//****************************************
// Locals
//****************************************

static int32_t r4aMenuUsers;

//*********************************************************************
// Constructor
// Inputs:
//   menu: Address of an R4A_MENU data structure
//   menuTable: Address of table containing the menu descriptions, the
//              main menu must be the first entry in the table.
//   menuEntries: Number of entries in the menu table
//   blankLineBeforePreMenu: Display a blank line before the preMenu
//   blankLineBeforeMenuHeader: Display a blank line before the menu header
//   blankLineAfterMenuHeader: Display a blank line after the menu header
//   alignCommands: Align the commands
//   blankLineAfterMenu: Display a blank line after the menu
void r4aMenuBegin(R4A_MENU * menu,
                  const R4A_MENU_TABLE * menuTable,
                  int menuEntries,
                  bool blankLineBeforePreMenu,
                  bool blankLineBeforeMenuHeader,
                  bool blankLineAfterMenuHeader,
                  bool alignCommands,
                  bool blankLineAfterMenu)
{
    // Initialize the R4A_MENU data structure
    log_v("r4aMenuBegin: menu %p", menu);
    memset(menu, 0, sizeof(*menu));
    menu->_menuTable = menuTable;
    menu->_menuTableEntries = menuEntries;
    menu->_blankLineBeforePreMenu = blankLineBeforePreMenu;
    menu->_blankLineBeforeMenuHeader = blankLineBeforeMenuHeader;
    menu->_blankLineAfterMenuHeader = blankLineAfterMenuHeader;
    menu->_alignCommands = alignCommands;
    menu->_blankLineAfterMenu = blankLineAfterMenu;
}

//*********************************************************************
// Display the boolean as enabled or disabled
void r4aMenuBoolHelp(const R4A_MENU_ENTRY * menuEntry, const char * align, Print * display)
{
    bool * value = (bool *)menuEntry->menuParameter;
    display->printf("%s: %s%s: %s\r\n", menuEntry->command, align,
                    menuEntry->helpText, *value ? "Enabled" : "Disabled");
}

//*********************************************************************
// Toggle boolean value
void r4aMenuBoolToggle(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display)
{
    bool * value = (bool *)menuEntry->menuParameter;
    *value ^= 1;
}

//*********************************************************************
// Display the menu object contents
void r4aMenuDisplay(R4A_MENU * menu, Print * display)
{
    display->printf("Menu @ %p\r\n", menu);
    display->printf("    _menu: %p\r\n", menu->_menu);
    display->printf("    _menuTable: %p\r\n", menu->_menuTable);
    display->printf("    _menuTableEntries: %d\r\n", menu->_menuTableEntries);
    display->printf("    _blankLineBeforePreMenu: %d\r\n", menu->_blankLineBeforePreMenu);
    display->printf("    _blankLineBeforeMenuHeader: %d\r\n", menu->_blankLineBeforeMenuHeader);
    display->printf("    _blankLineAfterMenuHeader: %d\r\n", menu->_blankLineAfterMenuHeader);
    display->printf("    _alignCommands: %d\r\n", menu->_alignCommands);
    display->printf("    _blankLineAfterMenu: %d\r\n", menu->_blankLineAfterMenu);
}

//*********************************************************************
// Get the string of parameters
String r4aMenuGetParameters(const struct _R4A_MENU_ENTRY * menuEntry,
                            const char * command)
{
    // Get the parameter name
    String line = String(&command[strlen(menuEntry->command)]);

    // Strip white space from the beginning and the end of the name
    line.trim();
    return line;
}

//*********************************************************************
// Display the menu item with a suffix and help text.  The suffix is
// specified as the menu parameter.
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   align: Zero terminated string of spaces for alignment
//   display: Device used for output
void r4aMenuHelpSuffix(const struct _R4A_MENU_ENTRY * menuEntry,
                       const char * align,
                       Print * display)
{
    display->printf("%s %s: %s%s\r\n",
                    menuEntry->command,
                    (const char *)menuEntry->menuParameter,
                    align,
                    menuEntry->helpText);
}

//*********************************************************************
// Determine if the menu system is active
// Returns true when the menu system is active and false when inactive
bool r4aMenuIsActive()
{
    return r4aMenuUsers ? true : false;
}

//*********************************************************************
// Process the menu command or display the menu
// Returns true when exiting the menu system
bool r4aMenuProcess(R4A_MENU * menu,
                    const char * command,
                    Print * display)
{
    String align("");
    int alignSpaces;
    const char * cmd;
    bool found;
    int length;
    int maxLength;
    const R4A_MENU_ENTRY * menuEnd;
    const R4A_MENU_ENTRY * menuEntry;
    const char * spaces = "                                                  ";
    int spaceCount;

    log_v("r4aMenuProcess: menu %p, command %p, display %p", menu, command, display);
    if (menu->_debug)
    {
        Serial.printf("command: %p %s%s%s\r\n",
                      command,
                      command ? "(" : "",
                      command ? command : "",
                      command ? ")" : "");
        Serial.printf("display: %p\r\n", display);
        Serial.printf("_menu: %p\r\n", menu->_menu);
    }

    // Always start with the main menu
    if (!menu->_menu)
    {
        // https://gcc.gnu.org/wiki/Atomic/GCCMM/LIbrary
        r4aAtomicAdd32(&r4aMenuUsers, 1);
        menu->_menu = &menu->_menuTable[0];
        if (menu->_debug)
            Serial.printf("_menu: %p\r\n", menu->_menu);
    }

    // Process the command
    if (command)
    {
        // Walk the menu table
        menuEntry = menu->_menu->firstEntry;
        menuEnd = &menuEntry[menu->_menu->menuEntryCount];
        while (menuEntry < menuEnd)
        {
            if (menu->_debug)
                Serial.printf("menuEntry: %p\r\n", menuEntry);

            // Determine if the command has parameters
            cmd = menuEntry->command;
            if (menu->_debug)
                Serial.printf("menuEntry->command: %p %s%s%s\r\n",
                              cmd,
                              cmd ? "(" : "",
                              cmd ? cmd : "",
                              cmd ? ")" : "");
            if (menuEntry->align)
            {
                length = strlen(cmd);
                found = (r4aStrincmp(command, cmd, length) == 0);
            }
            else
                found = (r4aStricmp(command, cmd) == 0);
            if (menu->_debug)
                Serial.printf("found: %d\r\n", found);

            // Determine if the command was found
            if (found)
            {
                if (menu->_debug)
                    Serial.printf("menuEntry->menuRoutine: %p\r\n", menuEntry->menuRoutine);

                // Process the command
                if (menuEntry->menuRoutine)
                {
                    if (menu->_debug)
                    {
                        Serial.printf("menuEntry: %p\r\n", menuEntry);
                        Serial.printf("command: %p %s%s%s\r\n",
                                      command,
                                      command ? "(" : "",
                                      command ? command : "",
                                      command ? ")" : "");
                        Serial.printf("display: %p\r\n", display);
                    }
                    menuEntry->menuRoutine(menuEntry, command, display);
                }
                else
                {
                    // Get the next menu index
                    uint32_t index = (int)menuEntry->menuParameter;
                    if (menu->_debug)
                    {
                        Serial.printf("index: %ld\r\n", index);
                        Serial.printf("_menuTableEntries: %d\r\n", menu->_menuTableEntries);
                    }

                    // Validate the next menu index
                    if (index <= menu->_menuTableEntries)
                    {
                        // Select the next menu
                        if (index)
                            menu->_menu = &menu->_menuTable[index - 1];

                        // Exit the menu system
                        else
                        {
                            // https://gcc.gnu.org/wiki/Atomic/GCCMM/LIbrary
                            r4aAtomicSub32(&r4aMenuUsers, 1);
                            menu->_menu = nullptr;
                        }
                        if (menu->_debug)
                            Serial.printf("_menu: %p\r\n", menu->_menu);
                    }
                    else
                        r4aReportFatalError("Invalid menu index!");
                }
                break;
            }
            menuEntry++;
        }

        // Display the error message
        if ((menuEntry >= menuEnd) && strlen(command))
            display->println("Error: Invalid command entered");
    }
    else
    {
        // Start at the beginning of the line
        display->print('\r');

        if (menu->_debug)
            Serial.printf("_menu->preMenu: %p\r\n", menu->_menu->preMenu);
        if (menu->_menu->preMenu)
        {
            // Separate the preMenu from previous output
            if (menu->_debug)
                Serial.printf("_blankLineBeforePreMenu: %d\r\n", menu->_blankLineBeforePreMenu);
            if (menu->_blankLineBeforePreMenu)
                display->println();

            // Display the data before the menu
            if (menu->_menu->preMenu(display) == false)
            {
                // Return to the main menu
                menu->_menu = &menu->_menuTable[0];
                if (menu->_debug)
                    Serial.printf("_menu: %p\r\n", menu->_menu);
                return false;
            }
        }

        // Separate the preMenu display from the menu header
        if (menu->_debug)
            Serial.printf("_blankLineBeforeMenuHeader: %d\r\n", menu->_blankLineBeforeMenuHeader);
        if (menu->_blankLineBeforeMenuHeader)
            display->println();

        // Display the menu header
        if (menu->_debug)
            Serial.printf("_menu->menuName: %p %s%s%s\r\n",
                          menu->_menu->menuName,
                          menu->_menu->menuName ? "(" : "",
                          menu->_menu->menuName ? menu->_menu->menuName : "",
                          menu->_menu->menuName ? ")" : "");
        display->printf("%s\r\n", menu->_menu->menuName);
        for (int length = strlen(menu->_menu->menuName); length > 0; length--)
            display->print("-");
        display->println();

        // Separate the menu header from the commands
        if (menu->_debug)
            Serial.printf("_blankLineAfterMenuHeader: %d\r\n", menu->_blankLineAfterMenuHeader);
        if (menu->_blankLineAfterMenuHeader)
            display->println();

        // Determine the maximum command length
        if (menu->_debug)
        {
            Serial.printf("_alignCommands: %d\r\n", menu->_alignCommands);
            Serial.printf("_menu->menuEntryCount: %ld\r\n", menu->_menu->menuEntryCount);
            Serial.printf("_menu->firstEntry: %p\r\n", menu->_menu->firstEntry);
        }
        maxLength = 0;
        if (menu->_alignCommands)
        {
            menuEntry = menu->_menu->firstEntry;
            menuEnd = &menuEntry[menu->_menu->menuEntryCount];
            while (menuEntry < menuEnd)
            {
                length = strlen(menuEntry->command) + menuEntry->align + 1;
                if (maxLength < length)
                    maxLength = length;
                menuEntry++;
            }
        }

        // Display the menu items
        menuEntry = menu->_menu->firstEntry;
        menuEnd = &menuEntry[menu->_menu->menuEntryCount];
        while (menuEntry < menuEnd)
        {
            if (menu->_debug)
            {
                Serial.printf("menuEntry: %p\r\n", menuEntry);
                Serial.printf("menuEntry->command: %p %s%s%s\r\n",
                              menuEntry->command,
                              menuEntry->command ? "(" : "",
                              menuEntry->command ? menuEntry->command : "",
                              menuEntry->command ? ")" : "");
                Serial.printf("menuEntry->align: %d\r\n", menuEntry->align);
                Serial.printf("menuEntry->helpRoutine: %p\r\n", menuEntry->helpRoutine);
            }

            // Align the menu items
            if (menu->_alignCommands)
            {
                align = String("");
                length = strlen(menuEntry->command) + menuEntry->align
                       + (menuEntry->align ? 1 : 0);
                spaceCount = strlen(spaces);
                alignSpaces = maxLength - length;
                while (alignSpaces > 0)
                {
                    length = alignSpaces;
                    if (length > spaceCount)
                        length = spaceCount;
                    align += &spaces[spaceCount - length];
                    alignSpaces -= length;
                }
            }

            // Display the menu item
            if (menuEntry->helpRoutine)
                menuEntry->helpRoutine(menuEntry, align.c_str(), display);
            else
                display->printf("%s: %s%s\r\n", menuEntry->command,
                                align.c_str(), menuEntry->helpText);
            menuEntry++;
        }

        // Separate the menu from any following text
        if (menu->_blankLineAfterMenu)
            display->println();
    }

    // Determine if exiting the menu system
    if (menu->_debug)
        Serial.printf("_menu: %p\r\n", menu->_menu);
    return (menu->_menu == nullptr);
}

//*********************************************************************
// Display data before the main menu header
bool mainMenuPre(Print * display)
{
    display->printf("Battery Voltage: %s Volts\r\n", batteryVoltageString);
    return true;
}

//*********************************************************************
// Process serial menu item
void r4aSerialMenu(R4A_MENU * menu)
{
    const char * command;
    String * line;
    static String serialBuffer;

    // Process input from the serial port
    line = r4aReadLine(true, &serialBuffer, &Serial);
    if (line)
    {
        // Get the command
        command = line->c_str();

        // Process the command
        if (!r4aMenuProcess(menu, command, &Serial))
            // Display the menu
            r4aMenuProcess(menu, nullptr, &Serial);

        // Start building the next command
        serialBuffer = "";
    }
}

//*********************************************************************
// Read a line of input from a Serial port into a String
String * r4aReadLine(bool echo,
                     String * buffer,
                     Stream * input,
                     Print * output)
{
    char data;
    String * line;

    // Wait for an input character
    line = nullptr;
    while (input->available())
    {
        // Get the input character
        data = input->read();
        if ((data != '\r') && (data != '\n'))
        {
            // Handle backspace
            if (data == 8)
            {
                // Output a bell when the buffer is empty
                if (buffer->length() <= 0)
                    output->write(7);
                else
                {
                    // Remove the character from the line
                    output->write(data);
                    output->write(' ');
                    output->write(data);

                    // Remove the character from the buffer
                    *buffer = buffer->substring(0, buffer->length() - 1);
                }
            }
            else
            {
                // Echo the character
                if (echo)
                    output->write(data);

                // Add the character to the line
                *buffer += data;
            }
        }

        // Echo a carriage return and linefeed
        else if (data == '\r')
        {
            if (echo)
                output->println();
            line = buffer;
            break;
        }

        // Echo the linefeed
        else if (echo && (data == '\n'))
            output->println();
    }

    // Return the line when it is complete
    return line;
}

//*********************************************************************
// Wait for the menu system to exit
void menuWaitExit()
{
    if (serialMenuRunning)
    {
        Serial.printf("Waiting for menu system to exit!\r\n");
        while (serialMenuRunning)
            delay(100);
    }
}

//****************************************
// Main menu
//****************************************

const R4A_MENU_ENTRY mainMenuTable[] =
{
    // Command  menuRoutine                     menuParam           HelpRoutine         align   HelpText
    {"d",     r4aEsp32NvmMenuDisplayParameters, 0,                  nullptr,            0,      "Display NVM parameters"},              //  2
    {"r",     r4aEsp32MenuSystemReset,          0,                  nullptr,            0,      "System reset"},
    {"s",       r4aEsp32NvmMenuParameterSet, (intptr_t)"pppp ssss", r4aMenuHelpSuffix,  9,      "Set parameter pppp value to xxxx"},    // 12
    {"t", r4aEsp32NvmMenuDisplayPartitionTable, 0,                  nullptr,            0,      "Display the partition table"},
    {"x",       nullptr,                        R4A_MENU_NONE,      nullptr,            0,      "Exit the menu system"},                // 15
    {"z",       r4aEsp32NvmMenuParameterClear,  (intptr_t)"pppp",   r4aMenuHelpSuffix,  4,      "Zero the parameter"},                  // 16
};
#define MAIN_MENU_ENTRIES       sizeof(mainMenuTable) / sizeof(mainMenuTable[0])

//****************************************
// List of menus
//****************************************

const R4A_MENU_TABLE menuTable[] =
{
    // menuName         preMenu routine firstEntry          entryCount
    {"Main Menu",       mainMenuPre,    mainMenuTable,      MAIN_MENU_ENTRIES},
};
const int menuTableEntries = sizeof(menuTable) / sizeof(menuTable[0]);
