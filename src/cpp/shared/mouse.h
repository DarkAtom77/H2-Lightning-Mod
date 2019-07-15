#ifndef MOUSE_H
#define MOUSE_H

class mouseManager {
public:
	char _[138];
	mouseManager();
	void ShowColorPointer();
	void __thiscall SetPointer(char*, int, int);
};

#endif