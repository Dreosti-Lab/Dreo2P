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
	amplitude_		= amplitude;
	input_rate_		= input_rate;
	output_rate_	= output_rate;
	x_pixels_		= x_pixels;
	y_pixels_		= y_pixels;
	num_chans_		= 2;

	// Initialize error
	int status = 0;

	// Generate scan pattern
	Generate_Scan_Waveform();

	// Create and start digital output task (shutter controller)
	DAQmxCreateTask("", &DO_taskHandle_);
	DAQmxCreateDOChan(DO_taskHandle_, "Dev1/port0/line0", "", DAQmx_Val_ChanPerLine);
	status = DAQmxStartTask(DO_taskHandle_);
	if (status) { Error_Handler(status, "DO Task setup"); }

	// Create analog input task
	DAQmxCreateTask("", &AI_taskHandle_);
	DAQmxCreateAIVoltageChan(AI_taskHandle_, "Dev1/ai0:1", "", DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts, NULL);
	status = DAQmxCfgSampClkTiming(AI_taskHandle_, "", input_rate_, DAQmx_Val_Rising, DAQmx_Val_ContSamps, samples_per_scan_);
	if (status) { Error_Handler(status, "AI Task setup"); }

	// Create analog output task
	DAQmxCreateTask("", &AO_taskHandle_);
	DAQmxCreateAOVoltageChan(AO_taskHandle_, "Dev1/ao0:1", "", -10.0, 10.0, DAQmx_Val_Volts, NULL);
	DAQmxCfgSampClkTiming(AO_taskHandle_, "", output_rate_, DAQmx_Val_Rising, DAQmx_Val_ContSamps, pixels_per_scan_);
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
	display.frame_data_A_ = Load_32f_1ch_Tiff_Frame_From_File("Dreo2P.tif", &image_width, &image_height);
	display.frame_data_B_ = Load_32f_1ch_Tiff_Frame_From_File("Dreo2P.tif", &image_width, &image_height);
	//display.frame_data_A_.resize(x_pixels_*y_pixels_);
	//display.frame_data_B_.resize(x_pixels_*y_pixels_);
	display.frame_width_ = image_width;
	display.frame_height_ = image_height;
	display.use_A_ = true;

	// Allocate space for analog input data
	int	buffer_size = (int)(input_rate_ * num_chans_); // Make buffer large enough to hold 1000 ms of 2 channel data
	double*	input_buffer = (double*) malloc(sizeof(double) * buffer_size);
	std::vector<float>	frame_ch0(pixels_per_frame_);
	std::vector<float>	frame_ch1(pixels_per_frame_);
	std::vector<float>	frame_display(pixels_per_frame_ * 4);

	// Declare helper local variables
	int	num_residual_samples = 0;
	int residual_sample_offset = 0;
	int32 num_read_samples = 0;
	int	num_new_samples = 0;
	int num_full_scan_lines = 0;
	int current_frame = 0;
	int current_line = 0;
	int	current_column = 0;
	float ch0_accum = 0.0f;
	float ch1_accum = 0.0f;
	int sample_ch0 = 0;
	int sample_ch1 = 0;
	bool first_scan = true;

	// Initialize error status
	int status = 0;

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
			first_scan = true;
		}

		// Scan acquisition loop
		current_frame = 0;
		current_line = 0;
		current_column = 0;
		while (scanning_)
		{
			// If first scan...open shutter and start acquisition
			if (first_scan)
			{
				// Open shutter
				Scanner::Set_Shutter_State(true);

				// Start hardware acqusition
				status = DAQmxStartTask(AI_taskHandle_);
				if (status) { Error_Handler(status, "AI Task start"); }
				std::cout << "Starting scanner.\n";

				// Reset first scan indicator
				first_scan = false;
			}

			// Read available input samples (on all channels)
			status = DAQmxReadAnalogF64(AI_taskHandle_, -1, 1.0, DAQmx_Val_GroupByScanNumber, &input_buffer[num_residual_samples*num_chans_], buffer_size, &num_read_samples, NULL);
			if (status) { Error_Handler(status, "AI Task read"); }
			
			// How many new samples (including left-over from previous scan)?
			num_new_samples = num_read_samples + num_residual_samples;

			// Measure number of full scan lines acquired and store residual (partial line) samples
			num_full_scan_lines = (int)floor(num_new_samples / samples_per_line_);

			// Extract samples for each channel from interleaved data array, bin, and sort into seperate frames (ignoring flyback)
			for (int i = 0; i < num_full_scan_lines; i++)
			{
				// Check if we reach the end of a comple frame.
				if (current_line == y_pixels_)
				{
					// Report progress
					std::cout << "Frame: " << current_frame;
					if ((current_line - 1) >= 0)
					{
						std::cout << frame_ch0[(current_line - 1)*x_pixels_] << " " << (current_line - 1) << "\n";
						display.intensity_ = (float)(current_line - 1) / 10.0f;
					}

					// Save data to TIFF file!!! update display stuff?

					// Reset scan_line pointer
					current_line = 0;

					// increment frame counter
					current_frame++;
				}
				// Loop though columns
				current_column = 0;
				for (int j = 0; j < (samples_per_line_*num_chans_); j += (num_chans_*bin_factor_))
				{
					// Bin subsequent samples into a pixel value
					ch0_accum = 0.0f;
					ch1_accum = 0.0f;
					for (int b = 0; b < bin_factor_*num_chans_; b+=num_chans_)
					{
						sample_ch0 = ((i*samples_per_line_*num_chans_) + j) + b;
						sample_ch1 = ((i*samples_per_line_*num_chans_) + j) + b + 1;

						// read input and split channels
						ch0_accum += (float)input_buffer[sample_ch0];
						ch1_accum += (float)input_buffer[sample_ch1];
					}
					// Save average pixel value in image frames
					if (current_column < x_pixels_)
					{
						frame_ch0[(current_line * x_pixels_) + current_column] = ch0_accum / (float)bin_factor_;
						frame_ch1[(current_line * x_pixels_) + current_column] = ch1_accum / (float)bin_factor_;
					}
					// Increment column counter
					current_column++;
				}
				// Increment scan line indicator
				current_line++;
			}

			// Append residual samples from previous read to input buffer
			num_residual_samples = num_new_samples - (num_full_scan_lines * samples_per_line_);
			residual_sample_offset = (num_full_scan_lines * samples_per_line_ * num_chans_);
			for (int r = 0; r < num_residual_samples*num_chans_; r+=num_chans_)
			{
				input_buffer[r] = input_buffer[residual_sample_offset + r];
				input_buffer[r + 1] = input_buffer[residual_sample_offset + r + 1];
			}

			// Update display frames (use double buffering!)
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

			// Sleep the thread for a bit (no need to update tooooooo quickly)
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

	// Free input buffer
	free(input_buffer);

	// Close GLFW window and thread
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
	status = DAQmxWriteAnalogF64(AO_taskHandle_, pixels_per_scan_, FALSE, 10.0, DAQmx_Val_GroupByScanNumber, scan_waveform_, NULL, NULL);
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
void Scanner::Generate_Scan_Waveform()
{
	// Check that input and out rates are multiples of one another
	if ((input_rate_ > output_rate_) && ((int)input_rate_ % (int)output_rate_ != 0))
	{
		Error_Handler(-1, "Input and output rate ratio must be positive integer.");
	}
	bin_factor_ = (int)input_rate_ / (int)output_rate_;

	// Number of backwards (return) pixels
	flyback_pixels_ = (int)floor(output_rate_ / 1000.0); // minimum 1 millisecond return

	// Compute forward scan velocity in volts/update (i.e. step size)
	double forward_velocity = (2.0 * amplitude_) / x_pixels_;

	// Perform Hermite blend interpolation from end of line to start of next line
	double *flyback = Scanner::Hermite_Blend_Interpolate(flyback_pixels_, amplitude_, -amplitude_, forward_velocity, forward_velocity);

	// Compute the size of each scan segment: forward and flyback (turn, backward, turn)
	pixels_per_line_ = x_pixels_ + flyback_pixels_;
	pixels_per_scan_ = pixels_per_line_ * y_pixels_;
	pixels_per_frame_ = x_pixels_ * y_pixels_;
	samples_per_line_ = pixels_per_line_ * bin_factor_;
	samples_per_scan_ = pixels_per_scan_ * bin_factor_;

	// Create space for scan waveform (both X and Y values)
	scan_waveform_ = (double *)malloc(sizeof(double) * pixels_per_scan_ * 2);

	// Fill array with scan positions (voltages)
	int offset = 0;
	for (size_t j = 0; j < y_pixels_; j++)
	{
		// Go from -amp to +amp in +velocity steps
		for (size_t i = 0; i < x_pixels_; i++)
		{
			// X value
			scan_waveform_[offset] = (-1.0 * amplitude_) + (forward_velocity * i);
			offset++;
			// Y value
			scan_waveform_[offset] = (-1.0 * amplitude_) + (forward_velocity * j);	// This may not make sense! (assumes X = Y)
			offset++;
		}
		// Then insert flyback from +amp to +amp
		double current_velocity = forward_velocity;
		double current_position = amplitude_;
		for (size_t i = 0; i < flyback_pixels_; i++)
		{
			// X value
			scan_waveform_[offset] = flyback[i];
			offset++;
			// Y value
			scan_waveform_[offset] = (-1.0 * amplitude_) + (forward_velocity * j);	// This may not make sense! (assumes X = Y)
			offset++;
		}
	}
	return;
}


// Save scan waveform to local file (for debugging) as CSV (this is very slow!)
void Scanner::Save_Scan_Waveform(std::string path, double* waveform)
{
	// Open file
	std::ofstream out_file;
	out_file.open(path, std::ios::out);
	
	// Write waveform data
	for (size_t i = 0; i < pixels_per_scan_; i++)
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
		fprintf(stderr, "Scnner Error (%i): %s\n", error, description);

		// Close gracefully
		Scanner::Close();

		// End application
		exit(error);
}

// FIN