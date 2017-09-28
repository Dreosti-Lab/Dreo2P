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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	Sleep(2000);

	// Stop scanning
	scanner.Stop();
	Sleep(100);
	scanner.color_ += 0.25f;

	// Start scanning
	scanner.Start();
	Sleep(3000);
=======
=======
=======
=======
	Sleep(500);

	// Stop scanning
	scanner.Stop();
	Sleep(250);

	// Start scanning
	scanner.Start();
>>>>>>> 1720eb56026af2bdb4215373ad575ff2ce34557b
	Sleep(500);

	// Stop scanning
	scanner.Stop();
	Sleep(250);

	// Start scanning
	scanner.Start();
<<<<<<< HEAD
>>>>>>> 1720eb56026af2bdb4215373ad575ff2ce34557b
	Sleep(500);

	// Stop scanning
	scanner.Stop();
	Sleep(250);

	// Start scanning
	scanner.Start();
<<<<<<< HEAD
>>>>>>> 1720eb56026af2bdb4215373ad575ff2ce34557b
	Sleep(500);

	// Stop scanning
	scanner.Stop();
	Sleep(250);

	// Start scanning
	scanner.Start();
	Sleep(500);
<<<<<<< HEAD

	// Stop scanning
	scanner.Stop();
	Sleep(250);

	// Start scanning
	scanner.Start();
	Sleep(500);
>>>>>>> 1720eb56026af2bdb4215373ad575ff2ce34557b
=======
>>>>>>> 1720eb56026af2bdb4215373ad575ff2ce34557b
=======
	Sleep(500);
>>>>>>> 1720eb56026af2bdb4215373ad575ff2ce34557b
=======
	Sleep(500);
>>>>>>> 1720eb56026af2bdb4215373ad575ff2ce34557b

	// Close
	scanner.Close();
	return 0;
}