// Dreo2P Console Application
#include <iostream>
#include <string>

#include "Scanner.h"

int main()
{
	std::cout << "Dreo2P Console Version\n";

	// Init scanner
	Scanner scanner = Scanner(1.0, 5000000, 10000, 100, 100);

	// Generate scan waveform
	std::cout << "Generating scan waveform...\n";
	double* waveform = scanner.Generate_Scan_Waveform();
	std::cout << "Done.\n";

	// Save scan waveform to a text file
	std::string save_path = "waveform.csv";
	std::cout << "Saving scan waveform...\n";
	scanner.Save_Scan_Waveform(save_path, waveform);
	std::cout << "Done.\n\n";

	// Report details
	std::cout << "X Pixels: " << scanner.x_pixels << "\n";
	std::cout << "Y Pixels: " << scanner.y_pixels << "\n";
	std::cout << "Flyback pixels: " << scanner.flyback_pixels << "\n";
	std::cout << "Pixels per frame: " << scanner.pixels_per_frame << "\n";

	return 0;
}