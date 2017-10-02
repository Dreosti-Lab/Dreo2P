// Dreo2PDLL.cpp 
// -------------------------------------------------------------------
// - A dead simple, high-performance scan acqusition system
// -- Assumes a NI PCI-6110 is installed as 'dev1': X is Ch0, Y is Ch1
// -- Assumes a Shutter is TTL controlled via dev1/port0/line0
// -------------------------------------------------------------------
// Author: Adam Kampff
#include <math.h>
#include <NIDAQmx.h>
#include "Scanner.h"
#include "Display.h"

// ---------------------
// Global Scanner Object 
// ---------------------
Scanner scanner;

// ---------------------
// Function Declarations
// ---------------------

// Externals
extern "C" __declspec(dllexport) void Initialize(
	double amplitude, 
	double input_rate, 
	double output_rate, 
	int x_pixels, 
	int y_pixels, 
	int averages, 
	int sample_shift,
	int num_to_save,
	char* path);

extern "C" __declspec(dllexport) void Start();
extern "C" __declspec(dllexport) void Configure_Display(int channel, float min, float max);
extern "C" __declspec(dllexport) void Stop();
extern "C" __declspec(dllexport) void Close();

// Open a console for error reporting and associate stdin/stdout with it!
// BOOL WINAPI AllocConsole(void);

// ---------------------------------
// External Function Definitions
// ---------------------------------

// Initialize
__declspec(dllexport) void Initialize(
		double amplitude, 
		double input_rate, 
		double output_rate, 
		int x_pixels, 
		int y_pixels, 
		int averages,
		int sample_shift,
		int num_to_save,
		char* path)
{
	// Configure file saving location
	scanner.Configure_Saving(path, num_to_save);

	// Initialize global scnner object
	scanner.Initialize(amplitude, input_rate, output_rate, x_pixels, y_pixels, averages, sample_shift);
	return;
}

// Start
__declspec(dllexport) void Start()
{
	// Start scanning (next average frame)
	scanner.Start();
}

// Configure display
__declspec(dllexport) void Configure_Display(int channel, float min, float max)
{
	// Update display parameters
	scanner.Configure_Display(channel, min, max);
}

// Stop
__declspec(dllexport) void Stop()
{
	// Stop scanning (interrupt saving or stop a continuous acquisition)
	scanner.Stop();
}


// Close
__declspec(dllexport) void Close()
{
	// Close the scanner
	scanner.Close();

	return;
}

// ---------------------------------
// Internal Function Definitions
// ---------------------------------

//FIN