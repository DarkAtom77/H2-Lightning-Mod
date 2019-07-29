#include<string>
#include<Windows.h>

#include "base.h"
#include "dialog.h"
#include "mouse.h"
#include "gui.h"
#include "spell\spell_constants.h"
#include "spell\spells.h"
#include "combat\creatures.h"

extern heroWindowManager *gpWindowManager;
extern int gbRemoteOn;
extern int giDialogTimeout;
extern heroWindow *pNormalDialogWindow;
extern int giResType1;
extern int giResExtra1;
extern int giResType2;
extern int giResExtra2;
extern font* bigFont;
extern mouseManager* gpMouseManager;
extern int __fastcall EventWindowHandler(struct tag_message &);
extern int __fastcall WaitHandler(struct tag_message &);


void H2MessageBox(char* msg) {
	if (msg) {
		char* str = new char[strlen(msg) + 1];
		str = strcpy(str, msg);
		NormalDialog(str, DIALOG_OKAY, -1, -1, -1, 0, -1, 0, -1, 0);
		delete str;
	}
}

void H2MessageBox(std::string &msg) {
  if (!msg.empty()) {
    H2MessageBox(&msg[0]);
  }
}

bool H2QuestionBox(char* msg) {
  NormalDialog(msg, DIALOG_YES_NO, -1,-1,-1,0,-1,0,-1,0);
  return gpWindowManager->buttonPressedCode != BUTTON_CODE_CANCEL;
}

int H2NormalDialog(char* msg, int yesno, int horizontal, int vertical, int img1type, int img1arg, int img2type, int img2arg, int writeOr)
{
	NormalDialog(msg, yesno, horizontal, vertical, img1type, img1arg, img2type, img2arg, writeOr, 0);
	return gpWindowManager->buttonPressedCode;
}

char* H2InputBox(char* msg, int len) {
  char* res = (char*) ALLOC(len+1);
  GetDataEntry(msg, res, len + 1, 0, 0, 1);
  return res;
}

void DisplayError(const char* msg, const char* title) {
	MessageBoxA(NULL, msg, title, MB_OK);
}

void DisplayError(std::string msg, std::string title) {
	MessageBoxA(NULL, msg.c_str(), title.c_str(), MB_OK);
}