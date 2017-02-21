
// Wrapper functions for OpenAutomate

// Add application info into registry key for Automate
// Return true if succeeded
bool AutomateInstallApp();

// Initialize OpenAutomate with its command line option. No need to cleanup
int AutomateInit(const char* opt);

// Call this function when the application is idle when IsAutomateMode() is true
void AutomateRun();

// Returns true if automate mode is initialized by AutomateInit() call
bool IsAutomateMode();

// Utility function to parse the command line option for OpenAutomate
// Returns OpenAutomate option if it has. If not, returns empty string
std::string GetAutomateOption(int argc, char* argv[]);

// Returns true if it has "-install" in the command line option
bool GetAutomateInstallModeOption(int argc, char* argv[]);

