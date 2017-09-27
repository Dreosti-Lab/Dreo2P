// Dreo2P Console Application
#include <iostream>
#include <string>

#include "Scanner.h"

int main()
{
	std::cout << "Dreo2P Console Version\n";
	std::cout << "----------------------\n";

	// Construct scanner
	Scanner scanner(3.0, 500000, 100000, 200, 200);

	// Start scanning
	scanner.Start();
	Sleep(500);

	// Stop scanning
	scanner.Stop();
	Sleep(250);

	// Start scanning
	scanner.Start();
	Sleep(500);

	// Stop scanning
	scanner.Stop();
	Sleep(250);

	// Start scanning
	scanner.Start();
	Sleep(500);

	// Close
	scanner.Close();
	return 0;
}