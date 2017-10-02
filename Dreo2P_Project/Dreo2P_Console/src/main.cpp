// Dreo2P Console Application

#include <iostream>
#include <string>

#include "Scanner.h"

int main()
{
	std::cout << "Dreo2P::Console Version\n";
	std::cout << "-----------------------\n";

	// Construct scanner
	Scanner scanner;
	int num_save = 5;
	scanner.Configure_Saving("Test", num_save);
	scanner.Initialize(1.0, 5000000.0, 250000.0, 512, 512, 1, 100);

	// Acquire a number of (averaged) frames
	for (size_t i = 0; i < num_save; i++)
	{
		// Start scanning
		scanner.Start();
		scanner.Configure_Display(0, -0.004f, 1.0f);

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