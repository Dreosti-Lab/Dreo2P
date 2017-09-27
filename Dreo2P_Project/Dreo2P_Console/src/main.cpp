// Dreo2P Console Application
#include <iostream>
#include <string>

#include "Scanner.h"

int main()
{
	std::cout << "Dreo2P Console Version\n";
	std::cout << "----------------------\n";

	// Construct scanner
	Scanner scanner(1.0, 500000, 100000, 200, 200);

	// Start scanning
	scanner.Start();
	Sleep(1000);

	// Stop scanning
	scanner.Stop();
	Sleep(1000);

	// Start scanning
	scanner.Start();
	Sleep(1000);

	// Close
	scanner.Close();
	return 0;
}