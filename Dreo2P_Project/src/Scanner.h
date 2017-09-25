// Dreo2P Scanner Class (header)
#pragma once
#include <math.h>
#include <NIDAQmx.h>
#include <string>
#include <fstream>


//#include "Display.h"

// Include STD headers
#include <iostream>

class Scanner
{
public:
	// Constructors
	Scanner();					// Create basic object reference
	Scanner(double _amplitude,	// Initialize scan parameters
			double _input_rate,
			double _output_rate,
			int _x_pixels,
			int _y_pixels);

	// Destructors
	~Scanner();

	// Members (and defaults)
	double amplitude = 1.0;			// Volts
	double input_rate = 5000000.0;	// Hz
	double output_rate = 250000.0;	// Hz
	int x_pixels = 1024;
	int y_pixels = 1024;
	int flyback_pixels = 512;
	int pixels_per_frame = -1;

	// Methods
	void Configure();
	void Start(double* scan_waveform);
	void Stop();
	void Close();
	double*	Generate_Scan_Waveform();
	void Save_Scan_Waveform(std::string path, double* waveform);

private:
	// Members

	// Methods
	double*	Hermite_Curve_Interpolate(int steps, double y1, double y2, double slope1, double slope2);
	static void Error_Callback(int error, const char* description);	// NIDAQ error callback function
};

