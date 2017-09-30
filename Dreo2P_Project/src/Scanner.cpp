// Dreo2P Scanner Class (source)

#include "Scanner.h"
#define _SCL_SECURE_NO_WARNINGS  

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
	// Open a GLFW (OpenGL) display window (on a seperate thread)
	Display display(1024, 1024);
	
	// Load the default image into memory shared with the display thread
	int image_width = 1024;
	int image_height = 1024;
	int num_chans = 2;
	display.frame_data_A_ = Load_32f_1ch_Tiff_Frame_From_File("Dreo2P.tif", &image_width, &image_height);
	display.frame_data_B_ = Load_32f_1ch_Tiff_Frame_From_File("Dreo2P.tif", &image_width, &image_height);
	//display.frame_data_A_.resize(x_pixels_*y_pixels_);
	//display.frame_data_B_.resize(x_pixels_*y_pixels_);
	display.frame_width_ = image_width;
	display.frame_height_ = image_height;
	display.use_A_ = true;

	// Allocate space for input data
	double*				input_buffer;
	std::vector<float>	frame_ch0(x_pixels_*y_pixels_);
	std::vector<float>	frame_ch1(x_pixels_*y_pixels_);
	std::vector<float>	frame_display(x_pixels_*y_pixels_*4);

	int	residual_samples = 0;
	int	buffer_size	= (int)round((sizeof(double) * input_rate_) / 2);
	input_buffer = (double *)malloc(buffer_size * 2); // Make buffer large enough to hold 100 ms of 2 channel data
	
	// Initialize status
	int status = 0;

	// Initial scan line counter
	int current_scan_line = 0;

	// Main thread loop...
	// -------------------
	while (active_)
	{
		// Reset mirror positions for next scan group
		Scanner::Reset_Mirrors();

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
			int32 num_read;
			status = DAQmxReadAnalogF64(AI_taskHandle_, -1, 1.0, DAQmx_Val_GroupByScanNumber, 
										&input_buffer[residual_samples*num_chans], buffer_size * 2, &num_read, NULL);
			if (status) { Error_Handler(status, "AI Task read"); }
			
			// How many new samples (including left-over from previous scan)?
			int new_samples = num_read + residual_samples;

			// Measure number of full scan lines acquired and store residual (partial line) samples
			int full_scan_lines = (int)floor(new_samples / samples_per_line_);

			// Extract samples for each channel from interleaved data array, bin and sort into frames
			for (int i = 0; i < full_scan_lines; i++)
			{
				// Check if we reach a new frame!!
				if (current_scan_line == y_pixels_)
				{
					// Save data to TIFF file!

					// Reset scan_line pointer
					current_scan_line = 0;
				}

				int current_col = 0;
				for (int j = 0; j < (samples_per_line_*num_chans); j += (num_chans*bin_factor_))
				{
					float ch0_accum = 0.0f;
					float ch1_accum = 0.0f;
					for (int b = 0; b < bin_factor_*num_chans; b+=num_chans)
					{
						int sample_ch0 = ((i*samples_per_line_*num_chans) + j) + b;
						int sample_ch1 = ((i*samples_per_line_*num_chans) + j) + b + 1;

						// read input and split channels
						ch0_accum += (float)input_buffer[sample_ch0];
						ch1_accum += (float)input_buffer[sample_ch1];
					}
					// Save average pixel value in image frames
					if (current_col < x_pixels_)
					{
						frame_ch0[(current_scan_line * x_pixels_) + current_col] = ch0_accum / (float)bin_factor_;
						frame_ch1[(current_scan_line * x_pixels_) + current_col] = ch1_accum / (float)bin_factor_;
					}
					// Increment column counter
					current_col++;
				}
				// Increment scan line indicator
				current_scan_line++;
			}

			// Append residual samples from previous read to input buffer
			residual_samples = new_samples - (full_scan_lines * samples_per_line_);
			int sample_offset = (full_scan_lines * samples_per_line_ * num_chans);

			for (int r = 0; r < residual_samples*num_chans; r+=num_chans)
			{
				input_buffer[r] = input_buffer[sample_offset + r];
				input_buffer[r + 1] = input_buffer[sample_offset + r + 1];
			}

			// Update display frame
			int frame_size = display.frame_width_ * display.frame_height_;
			if (display.use_A_)
			{
				std::copy(frame_ch0.begin(), frame_ch0.end(), display.frame_data_B_.begin());
				display.use_A_ = false;
			}
			else {
				std::copy(frame_ch0.begin(), frame_ch0.end(), display.frame_data_A_.begin());
				display.use_A_ = true;
			}
			display.frame_width_ = x_pixels_;
			display.frame_width_ = y_pixels_;

			// Increment frame counter
			frame_number++;
			std::cout << "Frame: " << frame_number;
			if (display.use_A_) 
			{
				std::cout << " A: ";
			}
			else {
				std::cout << " B: ";
			}
			if ((current_scan_line-1) >= 0)
			{
				std::cout << frame_ch0[(current_scan_line - 1)*x_pixels_] << " " << (current_scan_line - 1) << "\n";
				display.intensity_ = (float)(current_scan_line - 1) / 10.0f;
			}

			// Sleep the thread for a bit (no need to update tooo quickly)
			Sleep(16);
		}

		// Close shutter
		Scanner::Set_Shutter_State(false);

		// Stop analog input/output tasks
		DAQmxStopTask(AO_taskHandle_);
		status = DAQmxStopTask(AI_taskHandle_);
		if (status) { Error_Handler(status, "AI/AO Task stop"); }
		std::cout << "Stopping scanner.\n";
	}

	// Deallocate (thread) resources
	free(input_buffer);

	// Close GLFW window
	display.Close();

	return;
}


// Reset scanner
void Scanner::Reset_Mirrors()
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
	samples_per_line_ = (x_pixels_ + flyback_pixels_) * bin_factor_;

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


// Load a float32 grayscale (single channel) TIFF frame from a file
std::vector<float> Scanner::Load_32f_1ch_Tiff_Frame_From_File(char* path, int* width, int* height)
{
	// Open TIFF file at specified path
	TIFF* tif = TIFFOpen(path, "r");
	std::vector<float> frame;
	if (tif) {
		uint32 image_height;
		float* buf;
		uint32 row;
		uint32 col;

		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &image_height);
		uint32 image_width = (uint32)TIFFScanlineSize(tif)/4;

		// Allocate spcae for scan lines and full frame
		buf = (float*)malloc(sizeof(float)*image_width);
		frame.resize(image_width*image_height);
		
		// Load data one line at a time
		for (row = 0; row < image_height; row++)
		{
			TIFFReadScanline(tif, buf, row);
			for (col = 0; col < image_width; col++)
			{
				frame[col + (row*image_width)] = buf[col];
			}
		}
		free(buf);
		TIFFClose(tif);
		*width = image_width;
		*height = image_height;
	} else {
		*width = 0;
		*height = 0;
	}
	return frame;
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