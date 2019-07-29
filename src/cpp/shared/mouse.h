#ifndef MOUSE_H
#define MOUSE_H

#include "manager.h"
#include "resource\resources.h"

enum MOUSE_CURSOR_CATEGORY : int
{
	MOUSE_CURSOR_CATEGORY_ADVENTURE = 0x0,
	MOUSE_CURSOR_CATEGORY_COMBAT = 0x1,
	MOUSE_CURSOR_CATEGORY_SPELL = 0x2,
};

class mouseManager : public baseManager {
public:
	bitmap *bitmap;
	int spriteIdx;
	icon *cursorIcon;
	int cursorCategory;
	int cursorIdx;
	int field_4A;
	int field_4E;
	int field_52;
	int field_56;
	int field_5A;
	int cursorTopLeftX;
	int cursorTopLeftY;
	int field_66;
	int field_6A;
	int field_6E;
	int field_72;
	int cursorWidth;
	int cursorHeight;
	int field_7E;
	int couldBeShowMouse;
	int cursorDisabled;
	mouseManager();
	void ShowColorPointer();
	void __thiscall SetPointer(char*, int, int);
	
};

extern mouseManager* gpMouseManager;

#endif