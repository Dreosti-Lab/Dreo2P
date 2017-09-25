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
	flyback_pixels = (int)floor(output_rate / 1000.0); // minimum 1 millisecond return
	double ratio = x_pixels / flyback_pixels;

	// Compute scan velocities (forward and backward) in volts/update (i.e. step size)
	double forward_velocity = (2.0 * amplitude) / x_pixels;	// ...in volts/update

	// Perform Hermite blend interpolation from end to start
	double *flyback = Scanner::Hermite_Blend_Interpolate(flyback_pixels, amplitude, -amplitude, forward_velocity, forward_velocity);

	// Compute the size of each scan segment: forward and flyback (turn, backward, turn)
	int pixels_per_line = x_pixels + flyback_pixels;
	pixels_per_frame = pixels_per_line * y_pixels;

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
		// Then insert flyback from +amp to +amp
		double current_velocity = forward_velocity;
		double current_position = amplitude;
		for (size_t i = 0; i < flyback_pixels; i++)
		{
			scan_waveform[offset] = flyback[i];
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

// Scanner error callback function
void Scanner::Error_Callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}
