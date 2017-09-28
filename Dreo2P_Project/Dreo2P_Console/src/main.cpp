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
	Sleep(2000);

	// Start scanning
	scanner.Start();
	Sleep(2000);

	// Stop scanning
	scanner.Stop();
	Sleep(100);

	// Adjust display param
	scanner.color_ += 0.25f;

	// Start scanning
	scanner.Start();
	Sleep(2000);

	// Stop scanning
	scanner.Stop();
	Sleep(1000);

	// Close
	scanner.Close();
	return 0;
}