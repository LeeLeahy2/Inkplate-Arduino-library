/**
 **************************************************
 * @file        Menu.h
 * @brief       Serial menu support routine declarations
 *
 * @details     Code copied from R4A_ESP32 and R4A_Robot libraries.
 *
 * @author      Lee Leahy
 * @date        2026-11-06
 **************************************************/

#ifndef __MENU_H__
#define __MENU_H__

//----------------------------------------
// Menu API
//----------------------------------------

#define R4A_MENU_NONE           0
#define R4A_MENU_MAIN           1

// Process a menu item
// Inputs:
//   menuEntry: Address of the menu entry associated with the command
//   command: Full command line
//   display: Address of the Print object for output
typedef void (*R4A_MENU_ROUTINE)(const struct _R4A_MENU_ENTRY * menuEntry,
                                 const char * command,
                                 Print * display);

// Display help for a menu item
// Inputs:
//   menuEntry: Address of the menu entry to display help
//   align: Zero terminated string of spaces to align the help display
//   display: Address of the Print object for output
typedef void (*R4A_HELP_ROUTINE)(const struct _R4A_MENU_ENTRY * menuEntry,
                                 const char * align,
                                 Print * display);

typedef struct _R4A_MENU_ENTRY
{
    const char * command;         // Command: Expected serial input
    R4A_MENU_ROUTINE menuRoutine; // Routine to process the menu
    intptr_t menuParameter;       // Parameter for the menu routine
    R4A_HELP_ROUTINE helpRoutine; // Routine to display the help message
    int align;                    // Command length adjustment for alignment
    const char * helpText;        // Help text to display
} R4A_MENU_ENTRY;

// Forward declarations
// Inputs:
//   display: Address of the Print object for output
// Outputs:
//   Returns true if successful and false to force return to main menu
typedef bool (*R4A_PRE_MENU_DISPLAY)(Print * display);

typedef struct _R4A_MENU_TABLE
{
    // Menu description
    const char * menuName;             // Name of the menu
    R4A_PRE_MENU_DISPLAY preMenu;      // Routine to display data before the menu
    const R4A_MENU_ENTRY * firstEntry; // First entry in the menu
    int32_t menuEntryCount;            // Number of entries in the table
} R4A_MENU_TABLE;

typedef struct _R4A_MENU
{
    // private
    const R4A_MENU_TABLE * _menu;      // Current menu to display and use
    const R4A_MENU_TABLE * _menuTable; // Address of all menu descriptions
    int _menuTableEntries;             // Number of entries in the menu table

    // Menu formatting
    bool _blankLineBeforePreMenu;    // Display a blank line before the preMenu
    bool _blankLineBeforeMenuHeader; // Display a blank line before the menu header
    bool _blankLineAfterMenuHeader;  // Display a blank line after the menu header
    bool _alignCommands;             // Align the commands
    bool _blankLineAfterMenu;        // Display a blank line after the menu

    // public
    bool _debug; // Set true to enable debugging
} R4A_MENU;

extern const R4A_MENU_ENTRY mainMenuTable[];
extern const R4A_MENU_TABLE menuTable[];
extern const int menuTableEntries;

// Initialize the R4A_MENU data structure
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
                  bool blankLineBeforePreMenu = true,
                  bool blankLineBeforeMenuHeader = true,
                  bool blankLineAfterMenuHeader = false,
                  bool alignCommands = true,
                  bool blankLineAfterMenu = false);

// Display the boolean as enabled or disabled
// Inputs:
//   menuEntry: Address of the menu entry to display
//   align: Zero terminated string of spaces to align the help display
//   display: Address of the Print object for output
void r4aMenuBoolHelp(const R4A_MENU_ENTRY * menuEntry,
                     const char * align,
                     Print * display);

// Toggle boolean value
// Inputs:
//   menuEntry: Address of the menu entry containing the address to toggle
//   display: Address of the Print object for output
void r4aMenuBoolToggle(const R4A_MENU_ENTRY * menuEntry,
                       const char * command,
                       Print * display);

// Display the menu object contents
// Inputs:
//   menu: Address of an R4A_MENU data structure
//   display: Address of the Print object for output
void r4aMenuDisplay(R4A_MENU * menu, Print * display);

// Get the string of parameters
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
String r4aMenuGetParameters(const struct _R4A_MENU_ENTRY * menuEntry,
                            const char * command);

// Display the menu item with a suffix and help text
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   align: Zero terminated string of spaces for alignment
//   display: Device used for output
void r4aMenuHelpSuffix(const struct _R4A_MENU_ENTRY * menuEntry,
                       const char * align,
                       Print * display);

// Determine if the menu system is active
// Outputs:
//   Returns true when the menu system is active and false when inactive
bool r4aMenuIsActive();

// Process a menu command when specified or display the menu when command
// is nullptr.
// Inputs:
//   menu: Address of an R4A_MENU data structure
//   menuTable: Address of the address of the menuTable (description of the menu)
//   command: Command string
//   display: Address of the Print object for output
// Outputs:
//   True when exiting the menu system, false if still in the menu system
bool r4aMenuProcess(R4A_MENU * menu,
                    const char * command,
                    Print * display = &Serial);

//****************************************
// ReadLine API
//****************************************

// Read a line of input from a serial port into a character array
// Inputs:
//   echo: Specify true to enable echo of input characters and false otherwise
//   buffer: Address of a string that contains the input line
//   input: Address of a Stream structure
//   output: Address of a Print structure
// Outputs:
//   nullptr when the line is not complete
String * r4aReadLine(bool echo,
                     String * buffer,
                     Stream * input = &Serial,
                     Print * output = &Serial);

#endif  // __MENU_H__
