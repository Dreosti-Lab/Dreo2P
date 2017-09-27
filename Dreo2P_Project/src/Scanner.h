// Dreo2P Scanner Class (header)
#pragma once
// Include STD headers
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <math.h>

// Inlcude Local Headers
#include "windows.h"
#include "NIDAQmx.h"
#include "Display.h"

// #include libtiff

class Scanner
{
public:
	// Constructor
	Scanner(double _amplitude,
			double _input_rate,
			double _output_rate,
			int _x_pixels,
			int _y_pixels);

	// Destructors
	~Scanner();

	// Members
	Display display;
	double	amplitude;
	double	input_rate;
	double	output_rate;
	int		x_pixels;	
	int		y_pixels;
	int		flyback_pixels;
	int		pixels_per_frame;
	int		bin_factor;

	// Public Members (for acquisition thread)
	std::thread		scanner_thread;
	bool volatile	active = false;
	bool volatile	scanning = false;

	// Public Methods
	void Start();
	void Stop();
	void Reset();
	void Close();
	
private:

	// Private Members
	TaskHandle  DO_taskHandle = 0;
	TaskHandle  AO_taskHandle = 0;
	TaskHandle  AI_taskHandle = 0;
	double*		scan_waveform;

	// Thread Function Method
	void		ScannerThreadFunction();

	// Private Methods
	double*		Generate_Scan_Waveform();
	void		Set_Shutter_State(bool state);
	void		Save_Scan_Waveform(std::string path, double* waveform);
	double*		Hermite_Blend_Interpolate(int steps, double y1, double y2, double slope1, double slope2);
	void		Error_Handler(int error, const char* description);	// Scanner error handler function
};

