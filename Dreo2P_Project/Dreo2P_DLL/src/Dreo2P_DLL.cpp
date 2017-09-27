// Dreo2PDLL.cpp 
// -------------------------------------------------------------------
// - A dead simple, high-performance scan acqusition system
// -- Assumes a NI PCI-6110 is installed as 'dev1': X is Ch0, Y is Ch1
// -- Assumes a Shutter is TTL controlled via dev1/port0/line0
// -------------------------------------------------------------------
// Author: Adam Kampff
#include <math.h>
#include <NIDAQmx.h>
#include "Display.h"

// Global DLL Variables
//==========================================================
Display display;

// NIDAQ
static TaskHandle  AItaskHandle = 0, AOtaskHandle = 0;

// Thread

//-// Display Thread Variables
//HANDLE                      OGLDDLL_displayThread;
//DWORD                       OGLDDLL_displayThreadId;
//DWORD                       OGLDDLL_displayThreadParam;
//bool                        OGLDDLL_displaying = false;


// ---------------------
// Function Declarations
// ---------------------
// Externals
extern "C" __declspec(dllexport) int Initialize(double amplitude, double input_rate, double output_rate, int x_pixels, int y_pixels, int* flyback_pixels, int save);
extern "C" __declspec(dllexport) int Close();

// Internals
double*	Generate_Scan_Waveforms(double amplitude, double input_rate, double output_rate, int x_pixels, int y_pixels, int* flyback_pixels);
int		Initialize_Display(int width, int height);

// Initialize (generate scan waveform, configure NIDAQ, open window, open file)
// Start Scan (start scan acquisition, open shitter, start scan)
// Stop Scan  (stop scan acquisition, close shutter, stop scan)
// Close	  (close everything cleanly, file, NIDAQ, OpenGL)

// Open a console for error reporting and associate stdin/stdout with it!
// BOOL WINAPI AllocConsole(void);

// ---------------------------------
// External Function Definitions
// ---------------------------------

// Initialize
__declspec(dllexport) int Initialize(
		double amplitude, 
		double input_rate, 
		double output_rate, 
		int x_pixels, 
		int y_pixels, 
		int* flyback_pixels, 
		int save)
{
	// Poor man's error handling
	int ret = 0;

	// Configure NIDAQ: Simultanoues Analog Input/Output
	ret = DAQmxCreateTask("", &AItaskHandle);
	ret = DAQmxCreateAIVoltageChan(AItaskHandle, "Dev1/ai0", "", DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts, NULL);
	ret = DAQmxCfgSampClkTiming(AItaskHandle, "", 10000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1000);

	ret = DAQmxCreateTask("", &AOtaskHandle);
	ret = DAQmxCreateAOVoltageChan(AOtaskHandle, "Dev1/ao0", "", -10.0, 10.0, DAQmx_Val_Volts, NULL);
	ret = DAQmxCfgSampClkTiming(AOtaskHandle, "", 5000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1000);
	ret = DAQmxCfgDigEdgeStartTrig(AOtaskHandle, "Dev1/ai/StartTrigger", DAQmx_Val_Rising);


	//GenSineWave(1000, 1.0, 1.0 / 1000, &phase, AOdata);

	//ret = DAQmxWriteAnalogF64(AOtaskHandle, 1000, FALSE, 10.0, DAQmx_Val_GroupByChannel, AOdata, NULL, NULL);

	// Open an OpenGL window
	ret = Initialize_Display(x_pixels, y_pixels);

	// (if saving) Open a TIFF file

	// If success...
	return ret;
}

// Start


// Stop



// Close
__declspec(dllexport) int Close()
{
	// Close shutter

	// Stop NIDAQ

	// Close TIFF FIle

	// Close display window
	display.Close();
	return 1;
}

// ---------------------------------
// Internal Function Definitions
// ---------------------------------

// Generate the X and Y voltages for a raster scan pattern (unidirectional)
double* Generate_Scan_Waveforms(double amplitude, double input_rate, double output_rate, int x_pixels, int y_pixels, int* flyback_pixels)
{
	// Number of backwards (return) pixels
	int back_pixels = (int) floor(output_rate / 1000.0); // minimum 1 millsecond return

												   // Compute scan velocities (forward and backward)...in volts/update (i.e. step size)
	double forward_velocity = (2.0 * amplitude) / x_pixels;	// ...in volts/update
	double back_velocity = (2.0 * amplitude) / back_pixels;	// ...in volts/update 

															// Set turn acceleration and compute number of turn-around pixels
	double turn_acceleration = 0.00001;
	int turn_pixels = (int) floor((forward_velocity + back_velocity) / turn_acceleration);

	// Compute size of each scan segment: forward and flyback (turn, backward, turn)
	int pixels_per_line = x_pixels + turn_pixels + back_pixels + turn_pixels;
	int pixels_per_frame = pixels_per_line * y_pixels;
	*flyback_pixels = turn_pixels + back_pixels + turn_pixels; 	// Set number of flyback pixels

																// Create space for scan waveform (both X and Y values)
	double* scan_waveform;
	scan_waveform = (double *)malloc(sizeof(double) * pixels_per_frame * 2);

	// Fill array with scan positions (voltages)
	int offset = 0;
	// Go from -amp to +amp in +velocity steps
	for (size_t i = 0; i < x_pixels; i++)
	{
		scan_waveform[offset] = (-1.0 * amplitude) + (forward_velocity * i);
		offset++;
	}
	// Then go from +amp to +amp in decelerating velocity steps
	double turn_velocity = forward_velocity;
	double current_position = amplitude;
	for (size_t i = 0; i < turn_pixels; i++)
	{
		scan_waveform[offset] = current_position;
		turn_velocity -= turn_acceleration;
		current_position += turn_velocity;
		offset++;
	}
	// Then go from +amp to -amp in -back_velocity steps
	for (size_t i = 0; i < back_pixels; i++)
	{
		scan_waveform[offset] = (1.0 * amplitude) - (back_velocity * i);
		offset++;
	}
	turn_velocity = -back_velocity;
	for (size_t i = 0; i < turn_pixels; i++)
	{
		// Then go from amp to amp in velocity steps
	}

	return scan_waveform;
}

// Open scan display window
int Initialize_Display(int width, int height)
{
	// Intialize Render
	display.Initialize_Window(512, 512);
	display.Initialize_Render();

	return 1;
}
