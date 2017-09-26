// Dreo2P Console Application
#include <iostream>
#include <string>

#include "Scanner.h"

int main()
{
	std::cout << "Dreo2P Console Version\n";
	std::cout << "----------------------\n";

	// Construct scanner
	Scanner scanner(1.0, 5000000, 100000, 1000, 1000);

	// Start scanning
	scanner.Start();
	// Wait a bit
	Sleep(5000);
	// Stop scanning
	scanner.Stop();
	// Wait a bit
	Sleep(100);

	// Start scanning
	scanner.Start();
	// Wait a bit
	Sleep(5000);
	// Stop scanning
	scanner.Stop();
	// Wait a bit
	Sleep(100);

	// Close
	scanner.Close();
	return 0;
}