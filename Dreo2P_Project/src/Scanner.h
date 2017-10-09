// Dreo2P Scanner Class (header)

#pragma once
// Include STD headers
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <math.h>

// Inlcude Local Headers
#include "windows.h"
#include "tiffio.h"
#include "NIDAQmx.h"
#include "Display.h"

class Scanner
{
public:
	// Default Constructor
	Scanner();

	// Destructor
	~Scanner();

	// Public Methods
	void Initialize(double	amplitude,
					double	y_offset,
					double	input_rate,
					double	output_rate,
					int		x_pixels,
					int		y_pixels,
					int		frames_to_average_,
					int		sample_shift);
	void Start();
	void Stop();
	void Close();
	bool Is_Scanning();
	void Configure_Display(int channel, float min, float max, bool centre_cross, bool scan_line);
	void Configure_Saving(char *path, int images_to_save);

private:
	// Private Members (NIDAQmx)
	TaskHandle  DO_taskHandle_ = 0;
	TaskHandle  AO_taskHandle_ = 0;
	TaskHandle  AI_taskHandle_ = 0;

	// Private Members (scan parameters)
	double*	scan_waveform_;
	double	amplitude_;
	double	y_offset_;
	double	input_rate_;		// Number of samples per second
	double	output_rate_;		// Number of pixels per second
	int		bin_factor_;		// Ratio of samples per pixel (must be integer)
	int		num_chans_;
	int		x_pixels_;
	int		y_pixels_;
	int		flyback_pixels_;
	int		pixels_per_line_;
	int		pixels_per_scan_;
	int		pixels_per_frame_;
	int		samples_per_scan_;
	int		samples_per_line_;
	int		frames_to_average_;

	// Private members (display control)
	int					display_channel_ = 0;
	int					sample_shift_ = 0;
	float				min_ = 0.0f;
	float				max_ = 0.0f;
	bool				centre_cross_ = false;
	bool				scan_line_ = false;

	// Private members (TIFF)
	int					images_to_save_ = 0;
	std::string			file_path_;

	// Private Members (acquisition thread)
	std::thread			scanner_thread_;
	std::atomic<bool>	active_ = false;
	std::atomic<bool>	scanning_ = false;

	// Thread Function
	void				Scanner_Thread_Function();

	// Private Methods
	void				Reset_Mirrors();
	void				Generate_Scan_Waveform();
	void				Set_Shutter_State(bool state);
	void				Save_Scan_Waveform(std::string path, double* waveform);
	double*				Hermite_Blend_Interpolate(int steps, double y1, double y2, double slope1, double slope2);
	std::vector<float> 	Load_32f_1ch_Tiff_Frame_From_File(char* path, int* width, int* height);
	void				Save_Frame_to_32f_1ch_Tiff(TIFF *tiff_file, std::vector<float> data, int width, int height, int current_page, int total_pages);
	void				Error_Handler(int error, const char* description);	// Scanner error handler function
};

