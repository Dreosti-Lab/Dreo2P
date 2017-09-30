// Dreo2P Console Application
#include <iostream>
#include <string>

#include "Scanner.h"

int main()
{
	std::cout << "Dreo2P::Console Version\n";
	std::cout << "-----------------------\n";

	// Construct scanner
	Scanner scanner(3.0, 5000000, 100000, 1024, 1024);
	Sleep(1000);

	// Start scanning
	scanner.Start();
	Sleep(10000);

	// Stop scanning
	scanner.Stop();
	Sleep(1000);

	// Close
	scanner.Close();
	return 0;
}