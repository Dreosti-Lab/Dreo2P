// Dreo2P Scanner Class (source)
#include "Scanner.h"

// Constructor
Scanner::Scanner(
	double _amplitude,
	double _input_rate,
	double _output_rate,
	int _x_pixels,
	int _y_pixels)
{
	// Initialize scan parameters
	amplitude = _amplitude;
	input_rate = _input_rate;
	output_rate = _output_rate;
	x_pixels = _x_pixels;
	y_pixels = _y_pixels;

	// Start digital out (shutter controller)
	DAQmxCreateTask("", &DO_taskHandle);
	DAQmxCreateDOChan(DO_taskHandle, "Dev1/port0/line0", "", DAQmx_Val_ChanPerLine);
	DAQmxStartTask(DO_taskHandle);

	// Configure GLFW display
	display.Initialize_Window(256, 256);
	display.Initialize_Render();

	// Start the scan acquisition thread
	active = true;
	scanner_thread = std::thread(&Scanner::ScannerThreadFunction, this);
}

// Destructor
Scanner::~Scanner()
{
}

// Scanner thread function
void Scanner::ScannerThreadFunction()
{
	// While the scanner thread is active
	while (active)
	{
		// Reset scanner for next session
		std::cout << "Reset Scanner...\n";
		Reset();

		// Wait for scanning start signal
		while (!scanning && active)
		{
			Sleep(32);
			std::cout << "Waiting...\n";
		}

		// Open Shutter
		Set_Shutter_State(true);

		// Start hardware acqusition
		int res = DAQmxStartTask(AI_taskHandle);
		std::cout << res << "\n";

		double* photons;
		photons = (double *)malloc(sizeof(double) * pixels_per_frame);

		// Scan acquisition loop
		while (scanning)
		{
			Sleep(32);
			signed long num_read;
			DAQmxReadAnalogF64(AI_taskHandle, -1, 1.0, DAQmx_Val_GroupByChannel, photons, pixels_per_frame, &num_read, NULL);

			std::cout << "Read: " << num_read << "\n";

			// Get data bits, save, display, etc.
		}

		// Close Shutter
		Set_Shutter_State(false);

		// Close analog out
		DAQmxStopTask(AO_taskHandle);
		DAQmxClearTask(AO_taskHandle);

		// Close analog in
		DAQmxStopTask(AI_taskHandle);
		DAQmxClearTask(AI_taskHandle);
	}
	return;
}

// Reset scanner
void Scanner::Reset()
{
	// Generate scan pattern
	double *waveform = Generate_Scan_Waveform();
	double start_positions[2] = { waveform[0], waveform[1] };
	// std::cout << "Start X: " << start_positions[0] << "\n";
	// std::cout << "Start Y: " << start_positions[1] << "\n";

	// Set mirrors to start position
	DAQmxCreateTask("", &AO_taskHandle);
	DAQmxCreateAOVoltageChan(AO_taskHandle, "Dev1/ao0:1", "", -10.0, 10.0, DAQmx_Val_Volts, "");
	DAQmxStartTask(AO_taskHandle);
	DAQmxWriteAnalogF64(AO_taskHandle, 1, 1, 1.0, DAQmx_Val_GroupByChannel, start_positions, NULL, NULL);
	DAQmxWaitUntilTaskDone(AO_taskHandle, 1.0);
	DAQmxStopTask(AO_taskHandle);
	DAQmxClearTask(AO_taskHandle);

	// Load full scan parameters to hardware device (synchronous analog input/output)
	Load_Scan_Waveform(waveform);

	return;
}

// Start scanning
void Scanner::Start()
{
	// Start scanning loop (infinte or fixed number of frames)
	scanning = true;
}

// Stop scanning
void Scanner::Stop()
{
	// End scanning loop
	scanning = false;
}

// Close scanner
void Scanner::Close()
{
	// End scanning thread
	active = false;
	scanning = false;
	scanner_thread.join();

	// Close digital out (shutter controller)
	DAQmxStopTask(DO_taskHandle);
	DAQmxClearTask(DO_taskHandle);

	// Close GLFW window
	display.Close();
}

// Generate the X and Y voltages for a raster scan pattern (unidirectional)
double* Scanner::Generate_Scan_Waveform()
{
	// Number of backwards (return) pixels
	flyback_pixels = (int)floor(output_rate / 1000.0); // minimum 1 millisecond return
	double ratio = x_pixels / flyback_pixels;

	// Compute scan velocities (forward and backward) in volts/update (i.e. step size)
	double forward_velocity = (2.0 * amplitude) / x_pixels;	// ...in volts/update

	// Perform Hermite blend interpolation from end to start
	double *flyback = Scanner::Hermite_Blend_Interpolate(flyback_pixels, amplitude, -amplitude, forward_velocity, forward_velocity);

	// Compute the size of each scan segment: forward and flyback (turn, backward, turn)
	int pixels_per_line = x_pixels + flyback_pixels;
	pixels_per_frame = pixels_per_line * y_pixels;
	bin_factor = (int)round(input_rate / output_rate);		// This must be an integer (multiple)

	// Create space for scan waveform (both X and Y values)
	double* scan_waveform;
	scan_waveform = (double *)malloc(sizeof(double) * pixels_per_frame * 2);

	// Fill array with scan positions (voltages)
	int offset = 0;
	for (size_t j = 0; j < y_pixels; j++)
	{
		// Go from -amp to +amp in +velocity steps
		for (size_t i = 0; i < x_pixels; i++)
		{
			// X value
			scan_waveform[offset] = (-1.0 * amplitude) + (forward_velocity * i);
			offset++;
			// Y value
			scan_waveform[offset] = (-1.0 * amplitude) + (forward_velocity * j);	// This may not make sense! (assumes X = Y)
			offset++;
		}
		// Then insert flyback from +amp to +amp
		double current_velocity = forward_velocity;
		double current_position = amplitude;
		for (size_t i = 0; i < flyback_pixels; i++)
		{
			// X value
			scan_waveform[offset] = flyback[i];
			offset++;
			// Y value
			scan_waveform[offset] = (-1.0 * amplitude) + (forward_velocity * j);	// This may not make sense! (assumes X = Y)
			offset++;
		}
	}
	return scan_waveform;
}

// Send scan parameters to NIDAq device
void Scanner::Load_Scan_Waveform(double *waveform)
{
	// Setup input (read) operation
	DAQmxCreateTask("", &AI_taskHandle);
	DAQmxCreateAIVoltageChan(AI_taskHandle, "Dev1/ai0:1", "", DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts, NULL);
	DAQmxCfgSampClkTiming(AI_taskHandle, "", input_rate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, pixels_per_frame * bin_factor);

	// Setup output (write) operation
	std::cout << DAQmxCreateTask("", &AO_taskHandle);
	std::cout << "\n";
	std::cout << DAQmxCreateAOVoltageChan(AO_taskHandle, "Dev1/ao0:1", "", -10.0, 10.0, DAQmx_Val_Volts, NULL);
	std::cout << "\n";
	std::cout << DAQmxCfgSampClkTiming(AO_taskHandle, "", output_rate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, pixels_per_frame);
	std::cout << "\n";
	std::cout << DAQmxCfgDigEdgeStartTrig(AO_taskHandle, "/Dev1/ai/StartTrigger", DAQmx_Val_Rising);
	std::cout << "\n";
	std::cout << DAQmxStartTask(AO_taskHandle); //  Waits for analog input start trigger
	std::cout << "\n";
	std::cout << DAQmxWriteAnalogF64(AO_taskHandle, pixels_per_frame, FALSE, 30.0, DAQmx_Val_GroupByScanNumber, waveform, NULL, NULL);
	std::cout << "\n";

	return;
}

// Save scan waveform to local file (for debugging) as CSV (slow!)
void Scanner::Save_Scan_Waveform(std::string path, double* waveform)
{
	// Open file
	std::ofstream out_file;
	out_file.open(path, std::ios::out);
	
	// Write waveform data
	for (size_t i = 0; i < pixels_per_frame; i++)
	{
		out_file << waveform[i] << ',';
	}

	// Close file
	out_file.close();

	return;
}

// Helper Function: Blend interpolation
double* Scanner::Hermite_Blend_Interpolate(int steps, double y1, double y2, double slope1, double slope2)
{
	double* curve = (double*)malloc(sizeof(double)*steps);
	double next_y1 = y1;
	double next_y2 = y2 + (-slope2 * steps);
	for (size_t i = 0; i < steps; i++)
	{
		// Scale range from 0 to 1
		double s = (double)i / double(steps);

		// Compute Hermite basis functions
		double h1 = (2.0 * s*s*s) - (3.0 * s*s) + 1.0;
		double h2 = (-2.0 * s*s*s) + (3.0 * s*s);

		// Compute interpolated point
		double y = (h1 * next_y1) + (h2 * next_y2);
		curve[i] = y;

		// Increment
		next_y1 += slope1;
		next_y2 += slope2;
	}
	return curve;
}

// Control shutter state
void Scanner::Set_Shutter_State(bool state)
{
	// Set output data byte
	byte data[8] = { 0,0,0,0,0,0,0,0 };
	data[0] = (state ? 1 : 0);

	// Write shutter state
	DAQmxWriteDigitalU8(DO_taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);

	return;
}

// Scanner error callback function
void Scanner::Error_Callback(int error, const char* description)
{
	fprintf(stderr, "Error (%i): %s\n", error, description);
}
