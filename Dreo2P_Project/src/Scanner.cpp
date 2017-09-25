// Dreo2P Scanner Class (source)
#include "Scanner.h"

// Constructors
Scanner::Scanner()
{
}

Scanner::Scanner(
	double _amplitude,	// Initialize scan parameters
	double _input_rate,
	double _output_rate,
	int _x_pixels,
	int _y_pixels)
{
	amplitude = _amplitude;
	input_rate = _input_rate;
	output_rate = _output_rate;
	x_pixels = _x_pixels;
	y_pixels = _y_pixels;
}


// Destructor
Scanner::~Scanner()
{
}

// Initialize Scanner
void Scanner::Configure()
{
	return;
}

// Close window (and terminate GLFW)
void Scanner::Close()
{
	//glfwTerminate();
}


// Generate the X and Y voltages for a raster scan pattern (unidirectional)
double* Scanner::Generate_Scan_Waveform()
{
	// Number of backwards (return) pixels
	int back_pixels = (int)floor(output_rate / 1000.0); // minimum 1 millisecond return
	
	// Compute scan velocities (forward and backward) in volts/update (i.e. step size)
	double forward_velocity = (2.0 * amplitude) / x_pixels;	// ...in volts/update
	double back_velocity = (2.0 * amplitude) / back_pixels;	// ...in volts/update 

	// Set turn acceleration and compute number of turn-around pixels
	double turn_acceleration = 0.00001;
	int turn_pixels = (int)floor((forward_velocity + back_velocity) / turn_acceleration);

	// Compute the size of each scan segment: forward and flyback (turn, backward, turn)
	int pixels_per_line = x_pixels + turn_pixels + back_pixels + turn_pixels;
	pixels_per_frame = pixels_per_line * y_pixels;
	flyback_pixels = turn_pixels + back_pixels + turn_pixels; // Set number of flyback pixels

	// Create space for scan waveform (both X and Y values)
	double* scan_waveform;
	scan_waveform = (double *)malloc(sizeof(double) * pixels_per_frame * 2);

	// Fill array with scan positions (voltages)
	int offset = 0;
	for (size_t i = 0; i < y_pixels; i++)
	{
		// Go from -amp to +amp in +velocity steps
		for (size_t i = 0; i < x_pixels; i++)
		{
			scan_waveform[offset] = (-1.0 * amplitude) + (forward_velocity * i);
			offset++;
		}
		// Then go from +amp to +amp in decelerating velocity steps
		double current_velocity = forward_velocity;
		double current_position = amplitude;
		for (size_t i = 0; i < turn_pixels; i++)
		{
			scan_waveform[offset] = current_position;
			current_velocity -= turn_acceleration;
			current_position += current_velocity;
			offset++;
		}
		// Then go from +amp to -amp in -back_velocity steps
		for (size_t i = 0; i < back_pixels; i++)
		{
			scan_waveform[offset] = (1.0 * amplitude) - (back_velocity * i);
			offset++;
		}
		// Then go from -amp to -amp in accelerating velocity steps
		current_velocity = -back_velocity;
		current_position = -amplitude;
		for (size_t i = 0; i < turn_pixels; i++)
		{
			scan_waveform[offset] = current_position;
			current_velocity += turn_acceleration;
			current_position += current_velocity;
			offset++;
		}
	}
	return scan_waveform;
}

// Save scan waveform to file (for debugging)
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


// Scanner error callback function
void Scanner::Error_Callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}
