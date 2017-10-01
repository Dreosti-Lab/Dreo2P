// Dreo2P Console Application

#include <iostream>
#include <string>

#include "Scanner.h"

int main()
{
	std::cout << "Dreo2P::Console Version\n";
	std::cout << "-----------------------\n";

	// Construct scanner
	Scanner scanner(1.0, 5000000, 250000, 512, 512, 2);

	// Set save file name
	scanner.Configure_Saving("Test", 4);

	// Acquire a number of (averaged) frames
	for (size_t i = 0; i < 4; i++)
	{
		// Start scanning
		scanner.Start();
		scanner.Configure_Display(0);

		// Wait until done
		while (scanner.Is_Scanning())
		{
			// Maybe configure display or such...
			Sleep(32);

		}
		// Pause between scan groups
		Sleep(500);
	}

	// Close scanner
	scanner.Close();
	return 0;
}