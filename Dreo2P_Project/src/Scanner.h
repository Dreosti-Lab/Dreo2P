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
#include "tiffio.h"
#include "NIDAQmx.h"
#include "Display.h"

class Scanner
{
public:
	// Constructor
	Scanner(double	amplitude,
			double	input_rate,
			double	output_rate,
			int		x_pixels,
			int		y_pixels);

	// Destructors
	~Scanner();

	// Public members
	float color_ = 0.25f;

	// Public Methods
	void Start();
	void Stop();
	void Reset();
	void Close();
	
private:

	// Private Members (NIDAQmx)
	TaskHandle  DO_taskHandle_ = 0;
	TaskHandle  AO_taskHandle_ = 0;
	TaskHandle  AI_taskHandle_ = 0;

	// Private Members (scan parameters)
	double*	scan_waveform_;
	double	amplitude_;
	double	input_rate_;
	double	output_rate_;
	int		x_pixels_;
	int		y_pixels_;
	int		flyback_pixels_;
	int		pixels_per_frame_;
	int		bin_factor_;

	// Private Members (acquisition thread)
	std::thread		scanner_thread_;
	bool volatile	active_ = false;
	bool volatile	scanning_ = false;


	// Thread Function
	void		Scanner_Thread_Function();

	// Private Methods
	double*		Generate_Scan_Waveform();
	void		Set_Shutter_State(bool state);
	void		Save_Scan_Waveform(std::string path, double* waveform);
	double*		Hermite_Blend_Interpolate(int steps, double y1, double y2, double slope1, double slope2);
	float*		Load_Tiff_Frame_From_File(char* path);
	void		Error_Handler(int error, const char* description);	// Scanner error handler function
};

