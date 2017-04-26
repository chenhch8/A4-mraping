#include "wraping.h"

int main() {
	Wraping wrap;
	string dirname = "../TestData2/";
	string name = "";
	for (int i = 2; i < 14; ++i) {
		name = "";
		if (i > 9) {
			name += char(i / 10 + '0');
			name += char(i % 10 + '0');
		} else {
			name += char(i + '0');
		}
		wrap.start(dirname + name + ".bmp");
	}
	// wrap.start(dirname + "5.bmp");
	return 0;
}

// g++ main.cpp -O2 -L/usr/X11R6/lib -lm -lpthread -lX11
