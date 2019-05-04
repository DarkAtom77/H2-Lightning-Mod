#include "adventure/adv.h"
#include "adventure/map.h"
#include "artifacts.h"
#include "combat/creatures.h"
#include "combat/speed.h"
#include "game/game.h"
#include "gui/dialog.h"
#include "gui/gui.h"
#include "scripting/callback.h"
#include "sound/sound.h"
#include "spell/spells.h"
#include "prefs.h"
#include "skills.h"

#include "optional.hpp"
#include <sstream>
#include <string>
#include <cmath>

static const int END_TURN_BUTTON = 4;
unsigned char PlayerVisitedShrine[144][144] = { 0 };
static const std::string secondarySkillNames[] = {
	//Does this exist already elsewhere in the code?
	"Pathfinding",
	"Archery",
	"Logistics",
	"Scouting",
	"Diplomacy",
	"Navigation",
	"Leadership",
	"Wisdom",
	"Mysticism",
	"Luck",
	"Ballistics",
	"Eagle Eye",
	"Necromancy",
	"Estates",
};

int advManager::ProcessDeSelect(tag_message *evt, int *n, mapCell **cells) {
  extern int giBottomViewOverride;
  extern int iCurBottomView;
  extern int giBottomViewOverrideEndTime;

  if (evt->yCoordOrFieldID == END_TURN_BUTTON) {
    int hero_reminder_pref = read_pref<DWORD>("Show Hero Movement Reminder");
    //default is true, but read_pref() returns -1 if the value is not set
    bool show_hero_movement_reminder = hero_reminder_pref != 0;

    if (gpCurPlayer->HasMobileHero()) {
      if (!show_hero_movement_reminder) {
        gpGame->NextPlayer();
      } else { //if the movement reminder is on, ask player if he/she really wants to end turn
        NormalDialog("One or more heroes may still move, are you sure you want to end your turn?",
          2, -1, -1, -1, 0, -1, 0, -1, 0);

        if (gpWindowManager->buttonPressedCode != BUTTON_CODE_CANCEL)
          gpGame->NextPlayer();
      }
    } else { //there are no heroes with movement points left, end turn
      gpGame->NextPlayer();
    }

    // This is taken out of the original ProcessDeSelect. Exact purpose is unknown; could quite
    // possible be removed
    if (evt->yCoordOrFieldID >= 2000
      && evt->yCoordOrFieldID <= 2200) {
      if (giBottomViewOverride == 2) {
        giBottomViewOverride = 1;
      } else if (giBottomViewOverride) {
        giBottomViewOverride = 0;
      } else if (iCurBottomView == 2) {
        giBottomViewOverride = 1;
      } else {
        giBottomViewOverride = 2;
      }

      giBottomViewOverrideEndTime = KBTickCount() + 3000;
      UpdBottomView(1, 1, 1);
    }

    return 1;
  } else {
    return ProcessDeSelect_orig(evt, n, cells);
  }

}


int advManager::Open(int idx) {
  int res = this->Open_orig(idx);
  return res;
}

mapCell* advManager::MoveHero(int a2, int a3, int *a4, int *a5, int *a6, int a7, int *a8, int a9) {
  mapCell* res = MoveHero_orig(a2, a3, a4, a5, a6, a7, a8, a9);
  hero *hro = GetCurrentHero();
  ScriptCallback("OnHeroMove", hro->x, hro->y);
  return res;
}

void game::ShareVision(int sourcePlayer, int destPlayer) {
  this->sharePlayerVision[sourcePlayer][destPlayer] = true;
  this->PropagateVision();
}

void game::CancelShareVision(int sourcePlayer, int destPlayer) {
	this->sharePlayerVision[sourcePlayer][destPlayer] = false;
}

void game::PropagateVision() {
  for (int p1 = 0; p1 < NUM_PLAYERS; p1++) {
    for (int p2 = 0; p2 < NUM_PLAYERS; p2++) {
      if (this->sharePlayerVision[p1][p2]) {
        for (int i = 0; i < MAP_HEIGHT; i++) {
          for (int j = 0; j < MAP_WIDTH; j++) {
            if (MapCellVisible(j, i, p1)) {
              RevealMapCell(j, i, p2);
            }
          }
        }
      }
    }
  }
}

void game::SetVisibility(int x, int y, int player, int radius) {
  this->SetVisibility_orig(x, y, player, radius);

  for (int i = 0; i < NUM_PLAYERS; i++) {
    if (this->sharePlayerVision[player][i]) {
      // Would take more work to be transitive without infinite recursion
      this->SetVisibility_orig(x, y, i, radius);
    }
  }
}

void game::MakeAllWaterVisible(int player) {
  this->MakeAllWaterVisible_orig(player);

  for (int i = 0; i < NUM_PLAYERS; i++) {
    if (this->sharePlayerVision[player][i]) {
      // Would take more work to be transitive without infinite recursion
      this->MakeAllWaterVisible_orig(i);
    }
  }
}

void advManager::DoEvent(class mapCell *cell, int locX, int locY) {
  hero *hro = &gpGame->heroes[gpCurPlayer->curHeroIdx];
  int locationType = cell->objType & 0x7F;
  SAMPLE2 res2 = NULL_SAMPLE2;
  nonstd::optional<bool> shouldSkip = ScriptCallbackResult<bool>("OnLocationVisit", locationType, locX, locY);
  if (!shouldSkip.value_or(false)) {
	  if (locationType == LOCATION_SHRINE_FIRST_ORDER || locationType == LOCATION_SHRINE_SECOND_ORDER || locationType == LOCATION_SHRINE_THIRD_ORDER)
	  {
		  PlayerVisitedShrine[locX][locY] |= 1u << gpCurPlayer->color;
		  this->HandleSpellShrine(cell, locationType, hro, &res2, locX, locY);
	  }
	  else if (locationType == LOCATION_PYRAMID)
		  this->HandlePyramid(cell, locationType, hro, &res2, locX, locY);
	  else if (locationType == LOCATION_WITCH_HUT)
	  {
		  PlayerVisitedShrine[locX][locY] |= 1u << gpCurPlayer->color;
		  this->HandleWitchHut(cell, locationType, hro, &res2, locX, locY);
	  }
	  else
		  this->DoEvent_orig(cell, locX, locY);
	  ScriptCallback("AfterLocationVisit", locationType, locX, locY);
      return;
  }

  this->UpdateRadar(1, 0);
  this->UpdateHeroLocators(1, 1);
  this->UpdateTownLocators(1, 1);
  this->UpdBottomView(1, 1, 1);
  this->UpdateScreen(0, 0);
  ScriptCallback("AfterLocationVisit", locationType, locX, locY);
  gpSoundManager->SwitchAmbientMusic(giTerrainToMusicTrack[this->currentTerrain]);
  WaitEndSample(res2, res2.sample);
  CheckEndGame(0, 0);
}

void advManager::HandleSpellShrine(class mapCell *cell, int locationType, hero *hro, SAMPLE2* res2, int locX, int locY) {
  std::string shrineText;
  switch (locationType) {
    case LOCATION_SHRINE_FIRST_ORDER: {
      shrineText = "{Shrine of the 1st Circle}\n\nYou come across a small shrine attended by a group of novice acolytes.  In exchange for your protection, they agree to teach you a simple spell - '";
      shrineText += gSpellNames[cell->extraInfo - 1];
      shrineText += "'.  ";
      break;
    }
    case LOCATION_SHRINE_SECOND_ORDER: {
      shrineText = "{Shrine of the 2nd Circle}\n\nYou come across an ornate shrine attended by a group of rotund friars.  In exchange for your protection, they agree to teach you a spell - '";
      shrineText += gSpellNames[cell->extraInfo - 1];
      shrineText += "'.  ";
      break;
    }
    case LOCATION_SHRINE_THIRD_ORDER: {
      shrineText = "{Shrine of the 3rd Circle}\n\nYou come across a lavish shrine attended by a group of high priests.  In exchange for your protection, they agree to teach you a sophisticated spell - '";
      shrineText += gSpellNames[cell->extraInfo - 1];
      shrineText += "'.  ";
      break;
    }
  }

  if (hro->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
    if (gsSpellInfo[cell->extraInfo - 1].level > hro->secondarySkillLevel[SECONDARY_SKILL_WISDOM] + 2) {
      shrineText += "Unfortunately, you do not have the wisdom to understand the spell, and you are unable to learn it.";
	  int level = gsSpellInfo[cell->extraInfo - 1].level;
	  if (level == 4)
		  level = 1;
	  else if (level == 5)
		  level = 2;
	  else
		  level = 0;
      this->EventWindow(-1, 1, &shrineText[0], 8, cell->extraInfo - 1, 17, SECONDARY_SKILL_WISDOM * 3 + level, -1);
    } else {
      this->EventSound(locationType, NULL, res2);
      int heroKnowledge = hro->Stats(PRIMARY_SKILL_KNOWLEDGE);
      hro->AddSpell(cell->extraInfo - 1, heroKnowledge);
      this->EventWindow(-1, 1, &shrineText[0], 8, cell->extraInfo - 1, -1, 0, -1);
    }
  } else {
    shrineText += "Unfortunately, you have no Magic Book to record the spell with.";
    this->EventWindow(-1, 1, &shrineText[0], 8, cell->extraInfo - 1, 7, ARTIFACT_MAGIC_BOOK, -1);
  }
}

void advManager::HandlePyramid(class mapCell *cell, int locType, hero *hro, SAMPLE2 *res2, int locX, int locY) {
	this->EventSound(locType, cell->extraInfo, res2);

	this->EventWindow(-1, 2,
		"You come upon the pyramid of a great and ancient king.  You are tempted to search it for treasure, but all the old stories warn of fearful curses and undead guardians.  Will you search?",
		-1, 0, -1, 0, -1);

	if (gpWindowManager->buttonPressedCode == BUTTON_YES) {
		if (cell->extraInfo != 0) {

			if (!this->CombatMonsterEvent(hro, CREATURE_ROYAL_MUMMY, 30, cell, locX, locY, 0, locX, locY, CREATURE_VAMPIRE_LORD, 20, 2, -1, 0, 0)) {
				hro->CheckLevel();

				std::string msg;
				msg += "Upon defeating the monsters, you decipher an ancient glyph on the wall, telling the secret of the spell - '";
				msg += gSpellNames[cell->extraInfo - 1];
				msg += "'.  ";

				if (hro->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
					if (hro->secondarySkillLevel[SECONDARY_SKILL_WISDOM] < gsSpellInfo[cell->extraInfo - 1].level - 2) {
						msg += "  Unfortunately, you do not have the wisdom to understand the spell, and you are unable to learn it.  ";
						int level = gsSpellInfo[cell->extraInfo - 1].level;
						if (level == 4)
							level = 1;
						else if (level == 5)
							level = 2;
						else
							level = 0;
						advManager::EventWindow(-1, 1, &msg[0], 8, cell->extraInfo - 1, 17, SECONDARY_SKILL_WISDOM * 3 + level, -1);
					}
					else {
						int knowledge = hro->Stats(PRIMARY_SKILL_KNOWLEDGE);

						hro->AddSpell(cell->extraInfo - 1, knowledge);
						advManager::EventWindow(-1, 1, &msg[0], 8, cell->extraInfo - 1, -1, 0, -1);
					}
				}
				else {
					msg += "  Unfortunately, you have no Magic Book to record the spell with.";
					this->EventWindow(-1, 1, &msg[0], 8, cell->extraInfo - 1, 7, ARTIFACT_MAGIC_BOOK, -1);
				}
				cell->extraInfo = 0;
			}
		}
		else {
			NormalDialog(
				"You come upon the pyramid of a great and ancient king.  Routine exploration reveals that the pyramid is completely empty.",
				1, -1, -1, 11, 0, 11, 0, -1, 0);

			if (!(hro->flags & HERO_FLAG_RELATED_TO_PYRAMID)) {
				hro->flags |= HERO_FLAG_RELATED_TO_PYRAMID;
				hro->tempLuckBonuses -= 2;
			}
		}
	}
}

void advManager::HandleWitchHut(class mapCell *cell, int locType, hero *hro, SAMPLE2 *res2, int locX, int locY)
{
	int skill = cell->extraInfo;
	if (hro->GetSSLevel(skill) >= 1 || hro->numSecSkillsKnown >= NUM_SECONDARY_SKILLS)
	{
		DoEvent_orig(cell, locX, locY);
		return;
	}
	this->EventSound(locType, skill, res2);
	std::string str = "{Witch\'s Hut}\n\nYou enter a hut with bird's legs for stilts. The witch that lives inside wants to teach you a skill - ";
	str += secondarySkillNames[skill];
	str += ". Do you accept?";
	int answer = H2NormalDialog(&str[0], DIALOG_YES_NO, -1, -1, IMAGE_GROUP_SECONDARY_SKILLS, skill * 3, -1, 0, 0);
	if (answer == BUTTON_CODE_OKAY)
		hro->SetSS(skill, 1);
}

int advManager::MapPutArmy(int x, int y, int monIdx, int monQty) {
  int cellIdx = x * gpGame->map.height + y;
  gpGame->map.tiles[cellIdx].objectIndex = monIdx;
  gpGame->map.tiles[cellIdx].extraInfo = monQty;
  gpGame->map.tiles[cellIdx].objTileset = TILESET_MONSTER;
  gpGame->map.tiles[cellIdx].objType = TILE_HAS_EVENT | LOCATION_ARMY_CAMP;
  gpGame->map.tiles[cellIdx].overlayIndex = -1;
  gpGame->map.tiles[cellIdx].field_4_1 = 0;
  gpGame->map.tiles[cellIdx].isShadow = 0;
  return 0;
}

int mapCell::getLocationType() {
  return this->objType & 0x7F;
}

void advManager::QuickInfo(int x, int y) {
  const int xLoc = x + viewX;
  const int yLoc = y + viewY;
  if (!(xLoc >= 0 && xLoc < MAP_WIDTH && yLoc >= 0 && yLoc < MAP_HEIGHT)) {
    // Outside map boundary
    QuickInfo_orig(x, y);
    return;
  }

  const auto mapCell = GetCell(xLoc, yLoc);
  const int locationType = mapCell->objType & 0x7F;
  auto overrideText = ScriptCallbackResult<std::string>("GetTooltipText", locationType, xLoc, yLoc);
  if (!overrideText || overrideText->empty()) {
    // Lua error occurred or tooltip text not overridden.
	  if (locationType == LOCATION_SHRINE_FIRST_ORDER || locationType == LOCATION_SHRINE_SECOND_ORDER || locationType == LOCATION_SHRINE_THIRD_ORDER)
	  {
		  unsigned char visited = (PlayerVisitedShrine[xLoc][yLoc] >> gpCurPlayer->color) & 1u;
		  if (visited)
		  {
			  ShrineQuickInfo(xLoc, yLoc);
			  return;
		  }
	  }
	  else if (locationType == LOCATION_WITCH_HUT)
	  {
		  unsigned char visited = (PlayerVisitedShrine[xLoc][yLoc] >> gpCurPlayer->color) & 1u;
		  unsigned char visited2 = (PlayerVisitedShrine[xLoc][yLoc + 1] >> gpCurPlayer->color) & 1u;
		  //The Witch's Hut extends across 3 cells on the y-axis
		  //We only check for the bottom 2 because the player is
		  //REALLY unlikely to right-click the top one and it would
		  //create visual pollution
		  if (visited)
		  {
			  WitchHutQuickInfo(xLoc, yLoc);
			  return;
		  }
		  else if (visited2)
		  {
			  WitchHutQuickInfo(xLoc, yLoc + 1);
			  return;
		  }
	  }
	  else if (locationType == LOCATION_ARTIFACT)
	  {
		  ArtifactQuickInfo(xLoc, yLoc);
		  return;
	  }
    QuickInfo_orig(x, y);
    return;
  }

  // Ensure the tooltip box is visible on the screen.
  const int pTileSize = 32;
  const int pxOffset = -57;  // tooltip is drawn (-57,-25) pixels from the mouse
  const int pyOffset = -25;
  const int pTooltipWidth = 160;
  const int pTooltipHeight = 96;

  int px = pTileSize * x + pxOffset;
  if (px < 30) {
    // minimum indent from left edge
    px = 30;
  } else if (px + pTooltipWidth > 464) {
    // don't overrun right edge
    px = 304;
  }

  int py = pTileSize * y + pyOffset;
  if (py < 16) {
    // minimum indent from top edge
    py = 16;
  } else if (py + pTooltipHeight > 448) {
    // don't overrun bottom edge
    py = 352;
  }

  heroWindow tooltip(px, py, "qwikinfo.bin");
  GUISetText(&tooltip, 1, *overrideText);
  gpWindowManager->AddWindow(&tooltip, 1, -1);
  QuickViewWait();
  gpWindowManager->RemoveWindow(&tooltip);
}

void advManager::ShrineQuickInfo(int xLoc, int yLoc)
{
	const int x = xLoc - viewX;
	const int y = yLoc - viewY;

	// Ensure the tooltip box is visible on the screen.
	const int pTileSize = 32;
	const int pxOffset = -57;  // tooltip is drawn (-57,-25) pixels from the mouse
	const int pyOffset = -25;
	const int pTooltipWidth = 160;
	const int pTooltipHeight = 96;

	int px = pTileSize * x + pxOffset;
	if (px < 30) {
		// minimum indent from left edge
		px = 30;
	}
	else if (px + pTooltipWidth > 464) {
		// don't overrun right edge
		px = 304;
	}

	int py = pTileSize * y + pyOffset;
	if (py < 16) {
		// minimum indent from top edge
		py = 16;
	}
	else if (py + pTooltipHeight > 448) {
		// don't overrun bottom edge
		py = 352;
	}
	auto mapCell = GetCell(xLoc, yLoc);
	const int locationType = mapCell->objType & 0x7F;
	std::string str = "Shrine of the ";
	switch (locationType)
	{
		case LOCATION_SHRINE_FIRST_ORDER:
			str += "First Circle\n";
			break;
		case LOCATION_SHRINE_SECOND_ORDER:
			str += "Second Circle\n";
			break;
		case LOCATION_SHRINE_THIRD_ORDER:
			str += "Third Circle\n";
			break;
	}
	heroWindow tooltip(px, py, "qwikinfo.bin");
	std::string learned = "";
	if (gpCurPlayer->curHeroIdx != -1)
	{
		hero *hro = &gpGame->heroes[gpCurPlayer->curHeroIdx];
		if (hro != NULL)
			if (hro->spellsLearned[mapCell->extraInfo - 1] == 1)
				learned = "\n(already learned)";
			else
				learned = "\n(not learned)";
	}
	GUISetText(&tooltip, 1, &(std::string(str + "{Spell:} " + (std::string) gSpellNames[mapCell->extraInfo - 1] + learned))[0]);
	gpWindowManager->AddWindow(&tooltip, 1, -1);
	QuickViewWait();
	gpWindowManager->RemoveWindow(&tooltip);
}

void advManager::WitchHutQuickInfo(int xLoc, int yLoc)
{
	const int x = xLoc - viewX;
	const int y = yLoc - viewY;

	// Ensure the tooltip box is visible on the screen.
	const int pTileSize = 32;
	const int pxOffset = -57;  // tooltip is drawn (-57,-25) pixels from the mouse
	const int pyOffset = -25;
	const int pTooltipWidth = 160;
	const int pTooltipHeight = 96;

	int px = pTileSize * x + pxOffset;
	if (px < 30) {
		// minimum indent from left edge
		px = 30;
	}
	else if (px + pTooltipWidth > 464) {
		// don't overrun right edge
		px = 304;
	}

	int py = pTileSize * y + pyOffset;
	if (py < 16) {
		// minimum indent from top edge
		py = 16;
	}
	else if (py + pTooltipHeight > 448) {
		// don't overrun bottom edge
		py = 352;
	}
	auto mapCell = GetCell(xLoc, yLoc);
	const int locationType = mapCell->objType & 0x7F;
	std::string str = "Witch\'s Hut\n";
	heroWindow tooltip(px, py, "qwikinfo.bin");
	std::string learned = "";
	if (gpCurPlayer->curHeroIdx != -1)
	{
		hero *hro = &gpGame->heroes[gpCurPlayer->curHeroIdx];
		if (hro != NULL)
			if (hro->GetSSLevel(mapCell->extraInfo) >= 1)
				learned = "\n(already learned)";
			else
				learned = "\n(not learned)";
	}
	GUISetText(&tooltip, 1, &(std::string(str + "{Skill:} " + secondarySkillNames[mapCell->extraInfo] + learned))[0]);
	gpWindowManager->AddWindow(&tooltip, 1, -1);
	QuickViewWait();
	gpWindowManager->RemoveWindow(&tooltip);
}

void advManager::ArtifactQuickInfo(int xLoc, int yLoc)
{
	const int x = xLoc - viewX;
	const int y = yLoc - viewY;

	// Ensure the tooltip box is visible on the screen.
	const int pTileSize = 32;
	const int pxOffset = -57;  // tooltip is drawn (-57,-25) pixels from the mouse
	const int pyOffset = -25;
	const int pTooltipWidth = 160;
	const int pTooltipHeight = 96;

	int px = pTileSize * x + pxOffset;
	if (px < 30) {
		// minimum indent from left edge
		px = 30;
	}
	else if (px + pTooltipWidth > 464) {
		// don't overrun right edge
		px = 304;
	}

	int py = pTileSize * y + pyOffset;
	if (py < 16) {
		// minimum indent from top edge
		py = 16;
	}
	else if (py + pTooltipHeight > 448) {
		// don't overrun bottom edge
		py = 352;
	}
	auto mapCell = GetCell(xLoc, yLoc);
	const int locationType = mapCell->objType & 0x7F;
	int artifact = mapCell->objectIndex >> 1;
	if (artifact < 8 || artifact >= 64)
		artifact = LOBYTE(artifact) - 128;
	std::string str = GetArtifactName(artifact);
	heroWindow tooltip(px, py, "qwikinfo.bin");
	GUISetText(&tooltip, 1, &str[0]);
	gpWindowManager->AddWindow(&tooltip, 1, -1);
	QuickViewWait();
	gpWindowManager->RemoveWindow(&tooltip);
}

int advManager::ProcessSearch(int x, int y)
{
	//why is the name of this method so poorly chosen?
	//this is supposed to handle the Ultimate Artifact digging
	//I don't understand why the arguments are always -1
	hero* hro = &gpGame->heroes[gpCurPlayer->curHeroIdx];
	ScriptCallback("OnHeroDig", hro->x, hro->y);
	return ProcessSearch_orig(x, y);
}

int GetShrineSpell(int x, int y)
{
	auto cell = gpAdvManager->GetCell(x, y);
	const int locationType = cell->objType & 0x7F;
	if (locationType != LOCATION_SHRINE_FIRST_ORDER && locationType != LOCATION_SHRINE_SECOND_ORDER && locationType != LOCATION_SHRINE_THIRD_ORDER && locationType != LOCATION_PYRAMID)
		return -1;
	return cell->extraInfo - 1;
}

void SetShrineSpell(int x, int y, int spell)
{
	auto cell = gpAdvManager->GetCell(x, y);
	const int locationType = cell->objType & 0x7F;
	if (locationType == LOCATION_SHRINE_FIRST_ORDER || locationType == LOCATION_SHRINE_SECOND_ORDER || locationType == LOCATION_SHRINE_THIRD_ORDER || locationType == LOCATION_PYRAMID)
		cell->extraInfo = spell + 1;
}

int GetWitchHutSkill(int x, int y)
{
	auto cell = gpAdvManager->GetCell(x, y);
	const int locationType = cell->objType & 0x7F;
	if (locationType != LOCATION_WITCH_HUT)
		return -1;
	return cell->extraInfo;
}

void SetWitchHutSkill(int x, int y, int skill)
{
	auto cell = gpAdvManager->GetCell(x, y);
	const int locationType = cell->objType & 0x7F;
	if (locationType == LOCATION_WITCH_HUT)
		cell->extraInfo = skill;
}