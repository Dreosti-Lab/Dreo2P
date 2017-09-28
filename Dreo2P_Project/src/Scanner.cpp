// Dreo2P Scanner Class (source)
#include "Scanner.h"

// Constructor
Scanner::Scanner(
	double	amplitude,
	double	input_rate,
	double	output_rate,
	int		x_pixels,
	int		y_pixels)
{
	// Set scan parameters
	amplitude_ =	amplitude;
	input_rate_ =	input_rate;
	output_rate_ =	output_rate;
	x_pixels_ =		x_pixels;
	y_pixels_ =		y_pixels;

	// Initialize error
	int status = 0;

	// Generate scan pattern
	scan_waveform_ = Generate_Scan_Waveform();

	// Create and start digital output task (shutter controller)
	DAQmxCreateTask("", &DO_taskHandle_);
	DAQmxCreateDOChan(DO_taskHandle_, "Dev1/port0/line0", "", DAQmx_Val_ChanPerLine);
	status = DAQmxStartTask(DO_taskHandle_);
	if (status) { Error_Handler(status, "DO Task setup"); }

	// Create analog input task
	DAQmxCreateTask("", &AI_taskHandle_);
	DAQmxCreateAIVoltageChan(AI_taskHandle_, "Dev1/ai0:1", "", DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts, NULL);
	status = DAQmxCfgSampClkTiming(AI_taskHandle_, "", input_rate_, DAQmx_Val_Rising, DAQmx_Val_ContSamps, pixels_per_frame_ * bin_factor_);
	if (status) { Error_Handler(status, "AI Task setup"); }

	// Create analog output task
	DAQmxCreateTask("", &AO_taskHandle_);
	DAQmxCreateAOVoltageChan(AO_taskHandle_, "Dev1/ao0:1", "", -10.0, 10.0, DAQmx_Val_Volts, NULL);
	DAQmxCfgSampClkTiming(AO_taskHandle_, "", output_rate_, DAQmx_Val_Rising, DAQmx_Val_ContSamps, pixels_per_frame_);
	status = DAQmxCfgDigEdgeStartTrig(AO_taskHandle_, "/Dev1/ai/StartTrigger", DAQmx_Val_Rising);
	if (status) { Error_Handler(status, "AO Task setup"); }

	// Start the scan acquisition thread
	active_ = true;
	scanner_thread_ = std::thread(&Scanner::Scanner_Thread_Function, this);
}

// Destructor
Scanner::~Scanner()
{
}

// Scanner thread function
void Scanner::Scanner_Thread_Function()
{
	// Configure GLFW display (must be done in same thread as the rendering)
	Display display;
	display.Initialize_Window(512, 512);
	display.Initialize_Render();

	// Allocate space for input data
	double* input_buffer;
	double* ch0_buffer;
	double* ch1_buffer;
	int residual_samples = 0;

	int buffer_size = (sizeof(double) * input_rate_) / 10;
	input_buffer = (double *)malloc(buffer_size * 2);		// Make buffer large enough to hold 100 ms of 2 channel data
	ch0_buffer = (double *)malloc(buffer_size);				// Make buffer large enough to hold 100 ms of data
	ch1_buffer = (double *)malloc(buffer_size);				// Make buffer large enough to hold 100 ms of data

	// Initialize status
	int status = 0;

	// While the scanner thread is active
	while (active_)
	{
		// Reset mirror positions for next scan group
		Scanner::Reset();

		// Wait for start signal
		std::cout << "Waiting for start...";
		while (!scanning_ && active_)
		{
			Sleep(32);
		}

		// Scan acquisition loop
		int frame_number = 0;
		while (scanning_)
		{
			// If first scan...
			if (frame_number == 0)
			{
				// Open shutter
				Scanner::Set_Shutter_State(true);

				// Start hardware acqusition
				status = DAQmxStartTask(AI_taskHandle_);
				if (status) { Error_Handler(status, "AI Task start"); }
				std::cout << "Starting scanner.\n";
			}

			// Read available input samples (all channels)
			signed long num_read;
			status = DAQmxReadAnalogF64(AI_taskHandle_, -1, 1.0, DAQmx_Val_GroupByScanNumber, input_buffer, pixels_per_frame_, &num_read, NULL);
			if (status) { Error_Handler(status, "AI Task read"); }
			
			// Append residual samples from previous read


			// Seperate channels from raw input array and add after residual samples from previous frames
			for (size_t i = 0; i < num_read; i+=2)
			{
				ch0_buffer[i] = input_buffer[i*2];
				ch1_buffer[i] = input_buffer[(i*2)+1];
			}

			// Measure number of full scan lines acquired
			int samples_per_line = (x_pixels_ + flyback_pixels_) * bin_factor_;
			int num_scan_lines = floor(num_read / samples_per_line);
			residual_samples = num_read - (num_scan_lines * samples_per_line);


			// Report values read (summary)
			std::cout << "Residuals: " << residual_samples << "\n";

			// Display data
			display.Set_Frame((ch0_buffer[0]+3.0f)/3.0f);

			// Update texture
			display.Render();

			// Increment frame counter
			frame_number++;
		}

		// Close shutter
		Set_Shutter_State(false);

		// Stop analog input/output tasks
		DAQmxStopTask(AO_taskHandle_);
		status = DAQmxStopTask(AI_taskHandle_);
		if (status) { Error_Handler(status, "AI/AO Task stop"); }
		std::cout << "Stopping scanner.\n";
	}

	// Deallocate (thread) resources
	free(input_buffer);
	free(ch0_buffer);
	free(ch1_buffer);

	// Close GLFW window
	display.Close();

	return;
}


// Reset scanner
void Scanner::Reset()
{
	// Get scan start positions
	double start_positions[4] = { scan_waveform_[0], scan_waveform_[0], scan_waveform_[1], scan_waveform_[1] };
	int status = 0;

	// Set mirrors to start position (2 updates to flush buffer)
	status = DAQmxResetWriteOffset(AO_taskHandle_);
	if (status) { Error_Handler(status, "AO Write offset"); }
	DAQmxWriteAnalogF64(AO_taskHandle_, 2, 0, 1.0, DAQmx_Val_GroupByChannel, start_positions, NULL, NULL);
	DAQmxStartTask(AO_taskHandle_);
	DAQmxStartTask(AI_taskHandle_);
	DAQmxStopTask(AO_taskHandle_);
	status = DAQmxStopTask(AI_taskHandle_);
	if (status) { Error_Handler(status, "AI/AO Reset"); }

	// Load full scan parameters to AO hardware and restart device
	DAQmxResetWriteOffset(AO_taskHandle_);
	if (status) { Error_Handler(status, "AO Write offset"); }
	status = DAQmxWriteAnalogF64(AO_taskHandle_, pixels_per_frame_, FALSE, 10.0, DAQmx_Val_GroupByScanNumber, scan_waveform_, NULL, NULL);
	status = DAQmxStartTask(AO_taskHandle_);
	if (status) { Error_Handler(status, "AO Restart"); }

	return;
}


// Start scanning
void Scanner::Start()
{
	// Start scanning loop (infinte or fixed number of frames)
	scanning_ = true;
}


// Stop scanning
void Scanner::Stop()
{
	// End scanning loop
	scanning_ = false;
}


// Close scanner
void Scanner::Close()
{
	// Initialize status
	int status = 0;

	// End scanning thread (if active)
	if (active_)
	{
		active_ = false;
		scanning_ = false;
		scanner_thread_.join();
	}

	// Close NIDAQ tasks (if open)
	if (DO_taskHandle_ != 0) {
		DAQmxStopTask(DO_taskHandle_);
		DAQmxClearTask(DO_taskHandle_);
	}
	if (AO_taskHandle_ != 0) {
		DAQmxClearTask(AO_taskHandle_);
	}
	if (AI_taskHandle_ != 0) {
		DAQmxClearTask(AI_taskHandle_);
	}

	// Free resources
	free(scan_waveform_);
}


// Generate the X and Y voltages for a raster scan pattern (unidirectional)
double* Scanner::Generate_Scan_Waveform()
{
	// Number of backwards (return) pixels
	flyback_pixels_ = (int)floor(output_rate_ / 1000.0); // minimum 1 millisecond return
	double ratio = x_pixels_ / flyback_pixels_;

	// Compute scan velocities (forward and backward) in volts/update (i.e. step size)
	double forward_velocity = (2.0 * amplitude_) / x_pixels_;	// ...in volts/update

	// Perform Hermite blend interpolation from end to start
	double *flyback = Scanner::Hermite_Blend_Interpolate(flyback_pixels_, amplitude_, -amplitude_, forward_velocity, forward_velocity);

	// Compute the size of each scan segment: forward and flyback (turn, backward, turn)
	int pixels_per_line = x_pixels_ + flyback_pixels_;
	pixels_per_frame_ = pixels_per_line * y_pixels_;
	bin_factor_ = (int)round(input_rate_ / output_rate_);		// This must be an integer (multiple)

	// Create space for scan waveform (both X and Y values)
	double* scan_waveform;
	scan_waveform = (double *)malloc(sizeof(double) * pixels_per_frame_ * 2);

	// Fill array with scan positions (voltages)
	int offset = 0;
	for (size_t j = 0; j < y_pixels_; j++)
	{
		// Go from -amp to +amp in +velocity steps
		for (size_t i = 0; i < x_pixels_; i++)
		{
			// X value
			scan_waveform[offset] = (-1.0 * amplitude_) + (forward_velocity * i);
			offset++;
			// Y value
			scan_waveform[offset] = (-1.0 * amplitude_) + (forward_velocity * j);	// This may not make sense! (assumes X = Y)
			offset++;
		}
		// Then insert flyback from +amp to +amp
		double current_velocity = forward_velocity;
		double current_position = amplitude_;
		for (size_t i = 0; i < flyback_pixels_; i++)
		{
			// X value
			scan_waveform[offset] = flyback[i];
			offset++;
			// Y value
			scan_waveform[offset] = (-1.0 * amplitude_) + (forward_velocity * j);	// This may not make sense! (assumes X = Y)
			offset++;
		}
	}
	return scan_waveform;
}


// Save scan waveform to local file (for debugging) as CSV (slow!)
void Scanner::Save_Scan_Waveform(std::string path, double* waveform)
{
	// Open file
	std::ofstream out_file;
	out_file.open(path, std::ios::out);
	
	// Write waveform data
	for (size_t i = 0; i < pixels_per_frame_; i++)
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
	int status = DAQmxWriteDigitalU8(DO_taskHandle_, 1, 1, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);
	if (status) { Error_Handler(status, "DO Write"); }

	return;
}


// Scanner error callback function
void Scanner::Error_Handler(int error, const char* description)
{
		// Report error
		fprintf(stderr, "Error (%i): %s\n", error, description);

		// Close gracefully
		Scanner::Close();

		// End application
		exit(error);
}

// FIN