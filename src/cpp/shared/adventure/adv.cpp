#include "adventure/adv.h"
#include "adventure/map.h"
#include "artifacts.h"
#include "combat/creatures.h"
#include "combat/speed.h"
#include "game/game.h"
#include "gui/dialog.h"
#include "gui/gui.h"
#include "resource/resourceManager.h"
#include "scripting/callback.h"
#include "sound/sound.h"
#include "spell/spells.h"
#include "prefs.h"
#include "skills.h"
#include "terrain.h"

#include "optional.hpp"
#include <sstream>
#include <string>
#include <cmath>

char *adventureMapLocations[] = {
  " ",
  "Alchemist Lab",
  "Sign",
  "Buoy",
  "Skeleton",
  "Daemon Cave",
  "Treasure Chest",
  "Faerie Ring",
  "Campfire",
  "Fountain",
  "Gazebo",
  "Ancient Lamp",
  "Graveyard",
  "Archer's House",
  "Goblin Hut",
  "Dwarf Cottage",
  "Peasant Hut",
  "Log Cabin",
  "Road",
  "Event",
  "Dragon City",
  "Lighthouse",
  "Water Wheel",
  "Mine",
  "Army Camp",
  "Obelisk",
  "Oasis",
  "Resource",
  " ",
  "Sawmill",
  "Oracle",
  "Shrine of the First Circle",
  "Shipwreck",
  "Sea Chest",
  "Desert Tent",
  "Town",
  "Stone Liths",
  "Wagon Camp",
  "Well",
  "Whirlpool",
  "Windmill",
  "Artifact",
  "Hero",
  "Boat",
  " ",
  "Random Artifact",
  "Random Resource",
  "Random Monster",
  "Random Town",
  "Random Castle",
  " ",
  "Random Monster - weak",
  "Random Monster - medium",
  "Random Monster - strong",
  "Random Monster - very strong",
  "Random Hero",
  "Nothing Special",
  " ",
  "Watch Tower",
  "Tree House",
  "Tree City",
  "Ruins",
  "Fort",
  "Trading Post",
  "Abandoned Mine",
  "Dwarf Cabin",
  "Standing Stones",
  "Idol",
  "Tree of Knowledge",
  "Witch Doctor's Hut",
  "Temple",
  "Hill Fort",
  "Halfling Hole",
  "Mercenary Camp",
  "Shrine of the Second Circle",
  "Shrine of the Third Circle",
  "Pyramid",
  "City of the Dead",
  "Excavation",
  "Sphinx",
  "Wagon",
  "Tar Pit",
  "Artesian Spring",
  "Troll Bridge",
  "Watering Hole",
  "Witch's Hut",
  "Xanadu",
  "Cave",
  "Lean-To",
  "Magellan's Maps",
  "Flotsam",
  "Derelict Ship",
  "Shipwreck Survivor",
  "Bottle",
  "Magic Well",
  "Magic Garden",
  "Observation Tower",
  "Freeman's Foundry",
  "Stream",
  "Trees",
  "Mountains",
  "Volcano",
  "Flowers",
  "Rock",
  "Lake",
  "Mandrake",
  "Dead Tree",
  "Stump",
  "Crater",
  "Cactus",
  "Mound",
  "Dune",
  "Lava Pool",
  "Shrub",
  "Hole",
  "Outcropping",
  "Random Artifact - Treasure",
  "Random Artifact - Minor",
  "Random Artifact - Major",
  "%s Barrier",
  "%s Traveller's Tent",
  "%s",
  "%s",
  "Jail",
  "Shipyard"
};

static const int END_TURN_BUTTON = 4;
unsigned char PlayerVisitedObject[144][144] = { 0 };

int castleIconFrames[MAX_FACTIONS] = {
  9,
  10,
  11,
  12,
  13,
  14,
  0,
  0,
  0,
  0,
  0,
  0,
  25
};

int townIconFrames[MAX_FACTIONS] = {
  15,
  16,
  17,
  18,
  19,
  20,
  0,
  0,
  0,
  0,
  0,
  0,
  26
};

static const std::string secondarySkillNames[] = {
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

icon* ironfistHeroMapIcons[MAX_FACTIONS];

int advManager::ProcessDeSelect(tag_message *evt, int *n, mapCell **cells) {
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
  iCurBottomView = 0;
  this->field_37A = 0;
  bShowIt = 0;
  this->field_BA = 0;
  for(int i = 0; i < 12; ++i) {
    this->someComponents[0][i] = 0;
    this->someComponents[1][i] = 0;
  }
  if(!this->adventureScreen) {
    this->adventureScreen = new heroWindow(0, 0, "adv_wind.bin");
    if(!this->adventureScreen)
      MemError();
    this->heroScrollbarKnob = new iconWidget(540, 195, 8, 17, "scroll.icn", 4, 0, 26, 16, 1);
    if(!this->heroScrollbarKnob)
      MemError();
    this->adventureScreen->AddWidget(this->heroScrollbarKnob, -1);

    this->castleScrollbarKnob = new iconWidget(612, 195, 8, 17, "scroll.icn", 4, 0, 27, 16, 1);
    if(!this->castleScrollbarKnob)
      MemError();
    this->adventureScreen->AddWidget(this->castleScrollbarKnob, -1);
  }
  if(gbThisNetHumanPlayer[giCurPlayer])
    gpMouseManager->SetPointer("advmice.mse", 0, -999);
  else
    gpMouseManager->SetPointer("advmice.mse", 1, -999);
  if(!this->sizeOfSomethingMapRelated) {
    this->sizeOfSomethingMapRelated = (int)operator new(2 * MAP_HEIGHT * MAP_WIDTH);
    if(!this->sizeOfSomethingMapRelated)
      MemError();
  }
  this->field_A2 = 0;
  gpWindowManager->AddWindow(this->adventureScreen, 0, 1);
  if(!this->groundTileset)
    this->groundTileset = gpResourceManager->GetTileset("ground32.til");
  if(!this->clofTileset)
    this->clofTileset = gpResourceManager->GetTileset("clof32.til");
  if(!this->stonTileset)
    this->stonTileset = gpResourceManager->GetTileset("ston.til");
  if(!this->clopIcon)
    this->clopIcon = gpResourceManager->GetIcon("clop32.icn");
  for(int i = 0; i < NUM_TILESETS; ++i) {
    if(strlen(gTilesetFiles[i]) > 1 && !this->tilesetIcns[i] && i != TILESET_HERO) {
      if(i != TILESET_TOWN_RANDOM)
        this->tilesetIcns[i] = gpResourceManager->GetIcon(gTilesetFiles[i]);
    }
  }
  if(!this->heroIcons[0])
    this->heroIcons[0] = gpResourceManager->GetIcon("kngt32.icn");
  if(!this->heroIcons[1])
    this->heroIcons[1] = gpResourceManager->GetIcon("barb32.icn");
  if(!this->heroIcons[2])
    this->heroIcons[2] = gpResourceManager->GetIcon("sorc32.icn");
  if(!this->heroIcons[3])
    this->heroIcons[3] = gpResourceManager->GetIcon("wrlk32.icn");
  if(!this->heroIcons[4])
    this->heroIcons[4] = gpResourceManager->GetIcon("wzrd32.icn");
  if(!this->heroIcons[5])
    this->heroIcons[5] = gpResourceManager->GetIcon("necr32.icn");

  if(!this->boatIcon)
    this->boatIcon = gpResourceManager->GetIcon("boat32.icn");

  for(int i = 0; i < NUM_ORIG_FACTIONS; i++)
    ironfistHeroMapIcons[i] = this->heroIcons[i];
  ironfistHeroMapIcons[6] = this->boatIcon;
  if(!ironfistHeroMapIcons[FACTION_CYBORG])
    ironfistHeroMapIcons[FACTION_CYBORG] = gpResourceManager->GetIcon("cbrg32.icn");

  if(!this->frothIcon)
    this->frothIcon = gpResourceManager->GetIcon("froth.icn");
  gbLoadingMonoIcon = 1;
  if(!this->shadowIcon)
    this->shadowIcon = gpResourceManager->GetIcon("shadow32.icn");
  if(!this->boatShadowIcon)
    this->boatShadowIcon = gpResourceManager->GetIcon("boatshad.icn");
  gbLoadingMonoIcon = 0;
  if(!this->flagIconsHero[0])
    this->flagIconsHero[0] = gpResourceManager->GetIcon("b-flag32.icn");
  if(!this->flagIconsHero[1])
    this->flagIconsHero[1] = gpResourceManager->GetIcon("g-flag32.icn");
  if(!this->flagIconsHero[2])
    this->flagIconsHero[2] = gpResourceManager->GetIcon("r-flag32.icn");
  if(!this->flagIconsHero[3])
    this->flagIconsHero[3] = gpResourceManager->GetIcon("y-flag32.icn");
  if(!this->flagIconsHero[4])
    this->flagIconsHero[4] = gpResourceManager->GetIcon("o-flag32.icn");
  if(!this->flagIconsHero[5])
    this->flagIconsHero[5] = gpResourceManager->GetIcon("p-flag32.icn");
  if(!this->flagIconsBoat[0])
    this->flagIconsBoat[0] = gpResourceManager->GetIcon("b-bflg32.icn");
  if(!this->flagIconsBoat[1])
    this->flagIconsBoat[1] = gpResourceManager->GetIcon("g-bflg32.icn");
  if(!this->flagIconsBoat[2])
    this->flagIconsBoat[2] = gpResourceManager->GetIcon("r-bflg32.icn");
  if(!this->flagIconsBoat[3])
    this->flagIconsBoat[3] = gpResourceManager->GetIcon("y-bflg32.icn");
  if(!this->flagIconsBoat[4])
    this->flagIconsBoat[4] = gpResourceManager->GetIcon("o-bflg32.icn");
  if(!this->flagIconsBoat[5])
    this->flagIconsBoat[5] = gpResourceManager->GetIcon("p-bflg32.icn");
  gbLoadingMonoIcon = 1;
  if(!this->radarIcon)
    this->radarIcon = gpResourceManager->GetIcon("radar.icn");
  gbLoadingMonoIcon = 0;
  for(int i = 0; i < 28; ++i)
    this->loopSamples[i] = 0;
  for(int i = 0; i < 4; ++i) {
    this->field_2C2[i][0] = -1;
    this->field_2C2[i][1] = 127;
    this->field_2BE = 0;
  }
  this->GetCursorSampleSet(walkSpeed);
  if(gbThisNetHumanPlayer[giCurPlayer]) {
    SetNoDialogMenus(1);
  } else {
    gpGame->TurnOnAIMusic();
    SetNoDialogMenus(0);
  }
  glTimers = KBTickCount() + 120;
  int tmpVolume = soundVolume;
  if(soundVolume)
    soundVolume = 10;
  this->SetInitialMapOrigin();
  bShowIt = gbThisNetHumanPlayer[giCurPlayer];
  int tmpCurPlayer = giCurPlayer;
  int tmpbShowIt = bShowIt;
  giCurPlayer = giCurWatchPlayer;
  gpCurPlayer = &gpGame->players[giCurWatchPlayer];
  bShowIt = 1;
  this->RedrawAdvScreen(1, 0);
  giCurPlayer = tmpCurPlayer;
  bShowIt = tmpbShowIt;
  gpCurPlayer = &gpGame->players[tmpCurPlayer];
  if(!gbThisNetHumanPlayer[tmpCurPlayer])
    gpGame->ShowComputerScreen();
  KBChangeMenu(hmnuAdv);
  this->ForceNewHover();
  gpWindowManager->FadeScreen(0, 8, gPalette);
  giBottomViewOverride = 0;
  soundVolume = tmpVolume;
  gpSoundManager->AdjustSoundVolumes();
  this->type = MANAGER_TYPE_ADVMAP_MANAGER;
  this->idx = idx;
  this->ready = 1;
  strcpy(this->name, "advManager");
  return 0;
}

mapCell* advManager::MoveHero(int a2, int a3, int *a4, int *a5, int *a6, int a7, int *a8, int a9) {
  mapCell* res = MoveHero_orig(a2, a3, a4, a5, a6, a7, a8, a9);
  hero *hro = GetCurrentHero();
  ScriptCallback("OnHeroMove", hro->x, hro->y);
  return res;
}

void game::ForceComputerPlayerChase(hero *source, hero *dest, bool force) {
	this->forcedComputerPlayerChases[source->idx][dest->idx] = force;
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

    switch (locationType) {
    case LOCATION_SHRINE_FIRST_ORDER:
    case LOCATION_SHRINE_SECOND_ORDER:
    case LOCATION_SHRINE_THIRD_ORDER:
								PlayerVisitedObject[locX][locY] |= 1u << gpCurPlayer->color;
      this->HandleSpellShrine(cell, locationType, hro, &res2, locX, locY);
      break;
    case LOCATION_PYRAMID:
      this->HandlePyramid(cell, locationType, hro, &res2, locX, locY);
      break;
				case LOCATION_WITCH_HUT:
								PlayerVisitedObject[locX][locY] |= 1u << gpCurPlayer->color;
								this->HandleWitchHut(cell, locationType, hro, &res2, locX, locY);
								break;
    case LOCATION_SHIPYARD: {
      gpMouseManager->SetPointer(0);

      // check if there is place for a boat
      bool boatPossible = false;
      int boatX, boatY;
      std::pair<int, int> possibleCellOffsets[] = { {-1, -1}, { 0,-1 }, {1, -1}, {2, -1}, {2, 0}, {2, 1}, {1, 1}, {0, 1}, {-1, 1}, { -1, 0 } };
      for(auto offset : possibleCellOffsets) {
        boatX = locX + offset.first;
        boatY = locY + offset.second;
        mapCell *cell = gpAdvManager->GetCell(boatX, boatY);
        // check for water and absense of boats
        if(!giGroundToTerrain[cell->groundIndex] && !cell->objType) {
          boatPossible = true;
          break;
        }
      }

      if(gpGame->GetBoatsBuilt() >= MAX_BOATS || !boatPossible) {
        NormalDialog("Cannot build another boat.", 1, 208, 40, -1, 0, -1, 0, -1, 0);
      } else {        
        heroWindow* wind = new heroWindow(177, 20, "shipwind.bin");
        if(!wind)
          MemError();
        SetWinText(wind, 12);
        if(gpGame->players[giCurPlayer].resources[RESOURCE_GOLD] < 1000 || gpGame->players[giCurPlayer].resources[RESOURCE_WOOD] < 10) {
          GUIAddFlag(wind, BUTTON_OK, 4096);
          GUIRemoveFlag(wind, BUTTON_OK, 2);
        }
        gpWindowManager->DoDialog(wind, TrueFalseDialogHandler, 0);
        delete wind;

        if(gpWindowManager->buttonPressedCode == BUTTON_OK) {
          if(gpGame->CreateBoat(boatX, boatY, 0) == -1) {
            LogStr("Can't create boat!");
          } else {
            gpGame->players[giCurPlayer].resources[RESOURCE_GOLD] -= 1000;
            gpGame->players[giCurPlayer].resources[RESOURCE_WOOD] -= 10;

            if(bShowIt) {
              gpMouseManager->HideColorPointer();
              gpWindowManager->SaveFizzleSource(168, 160, 176, 132);
              this->CompleteDraw(0);
              gpWindowManager->FizzleForward(168, 160, 176, 132, 65, 0, 0);
              gpMouseManager->ShowColorPointer();
            }
          }
        }
      }
      break;
    }
				case LOCATION_ALCHEMIST_TOWER:
								switch (cell->extraInfo)
								{
								case 0: //Alchemist Tower
												this->HandleAlchemistTower(cell, locationType, hro, &res2, locX, locY);
												break;
								case 1: //Arena
												this->HandleArena(cell, locationType, hro, &res2, locX, locY);
												break;
								default:
												this->DoEvent_orig(cell, locX, locY);
												break;
								}
								break;
    default:
      this->DoEvent_orig(cell, locX, locY);
						ScriptCallback("OnAfterLocationVisit", locationType, locX, locY);
      return;
    }
  }

  this->UpdateRadar(1, 0);
  this->UpdateHeroLocators(1, 1);
  this->UpdateTownLocators(1, 1);
  this->UpdBottomView(1, 1, 1);
  this->UpdateScreen(0, 0);
  gpSoundManager->SwitchAmbientMusic(giTerrainToMusicTrack[this->currentTerrain]);
  WaitEndSample(res2, res2.sample);
  CheckEndGame(0, 0);
}

void advManager::DoAIEvent(class mapCell * cell, class hero *hro, int locX, int locY)
{
				int locationType = cell->objType & 0x7F;
				SAMPLE2 res2 = NULL_SAMPLE2;
				nonstd::optional<bool> shouldSkip = ScriptCallbackResult<bool>("OnLocationVisit", locationType, locX, locY);
				playerData* currentPlayer = &gpGame->players[hro->ownerIdx];
				if (!shouldSkip.value_or(false))
				{
								switch (locationType)
								{
								case LOCATION_ALCHEMIST_TOWER:
												switch (cell->extraInfo)
												{
												case 0: //Alchemist Tower
																this->HandleAlchemistTower(cell, locationType, hro, &res2, locX, locY);
																break;
												case 1: //Arena
																this->HandleArena(cell, locationType, hro, &res2, locX, locY);
																break;
												default:
																this->DoAIEvent_orig(cell, hro, locX, locY);
																break;
												}
												break;
								case LOCATION_SHRINE_FIRST_ORDER:
								case LOCATION_SHRINE_SECOND_ORDER:
								case LOCATION_SHRINE_THIRD_ORDER:
								case LOCATION_WITCH_HUT:
												PlayerVisitedObject[locX][locY] |= 1u << currentPlayer->color;
								default:
												this->DoAIEvent_orig(cell, hro, locX, locY);
												break;
								}
								ScriptCallback("OnAfterLocationVisit", locationType, locX, locY);
								return;
				}
				CheckEndGame(0, 0);
				return;
}

void __thiscall game::ClaimTown(int castleidx, int playeridx, int a4) {
	this->ClaimTown_orig(castleidx, playeridx, a4);
	ScriptCallback("OnCastleConquered", castleidx , playeridx);
}

void advManager::HandleSpellShrine(class mapCell *cell, int locationType, hero *hro, SAMPLE2 *res2, int locX, int locY) {
  int spellId = cell->extraInfo - 1;

  std::string shrineText;
  switch (locationType) {
    case LOCATION_SHRINE_FIRST_ORDER: {
      shrineText = "{Shrine of the 1st Circle}\n\nYou come across a small shrine attended by a group of novice acolytes.  In exchange for your protection, they agree to teach you a simple spell - '";
      shrineText += gSpellNames[spellId];
      shrineText += "'.  ";
      break;
    }
    case LOCATION_SHRINE_SECOND_ORDER: {
      shrineText = "{Shrine of the 2nd Circle}\n\nYou come across an ornate shrine attended by a group of rotund friars.  In exchange for your protection, they agree to teach you a spell - '";
      shrineText += gSpellNames[spellId];
      shrineText += "'.  ";
      break;
    }
    case LOCATION_SHRINE_THIRD_ORDER: {
      shrineText = "{Shrine of the 3rd Circle}\n\nYou come across a lavish shrine attended by a group of high priests.  In exchange for your protection, they agree to teach you a sophisticated spell - '";
      shrineText += gSpellNames[spellId];
      shrineText += "'.  ";
      break;
    }
  }

  if (hro->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
    if (gsSpellInfo[spellId].level > hro->secondarySkillLevel[SECONDARY_SKILL_WISDOM] + 2) {
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
      hro->AddSpell(spellId, heroKnowledge);
      this->EventWindow(-1, 1, &shrineText[0], 8, spellId, -1, 0, -1);
    }
  } else {
    shrineText += "Unfortunately, you have no Magic Book to record the spell with.";
				this->EventWindow(-1, 1, &shrineText[0], 8, cell->extraInfo - 1, 7, ARTIFACT_MAGIC_BOOK, -1);
  }
}

void advManager::HandlePyramid(class mapCell *cell,int locType, hero *hro, SAMPLE2 *res2, int locX, int locY) {
  int spellId = cell->extraInfo - 1;

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
        msg += gSpellNames[spellId];
        msg += "'.  ";

        if (hro->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
          if (hro->secondarySkillLevel[SECONDARY_SKILL_WISDOM] < gsSpellInfo[spellId].level - 2) {
            msg += "  Unfortunately, you do not have the wisdom to understand the spell, and you are unable to learn it.  ";
												int level = gsSpellInfo[cell->extraInfo - 1].level;
												if (level == 4)
																level = 1;
												else if (level == 5)
																level = 2;
												else
																level = 0;
												advManager::EventWindow(-1, 1, &msg[0], 8, cell->extraInfo - 1, 17, SECONDARY_SKILL_WISDOM * 3 + level, -1);
          } else {
            int knowledge = hro->Stats(PRIMARY_SKILL_KNOWLEDGE);

            hro->AddSpell(spellId, knowledge);
            advManager::EventWindow(-1, 1, &msg[0], 8, spellId, -1, 0, -1);
          }
        } else {
          msg += "  Unfortunately, you have no Magic Book to record the spell with.";
										this->EventWindow(-1, 1, &msg[0], 8, cell->extraInfo - 1, 7, ARTIFACT_MAGIC_BOOK, -1);
        }
        cell->extraInfo = 0;
      }
    } else {
      NormalDialog(
        "You come upon the pyramid of a great and ancient king.  Routine exploration reveals that the pyramid is completely empty.",
        1, -1, -1,11, 0, 11, 0, -1, 0);

      if (!(hro->flags & HERO_PYRAMID_RAIDED)) {
        hro->flags |= HERO_PYRAMID_RAIDED;
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

void advManager::HandleArena(class mapCell *cell, int locType, hero *hro, SAMPLE2 *res2, int locX, int locY)
{
				HeroExtraII* hero_extra = HeroExtras[hro->idx];
				if (hero_extra->HasVisitedArena(locX, locY))
								hro->flags |= HERO_ARENA_VISITED;
				else
								hro->flags &= ~(HERO_ARENA_VISITED);
				if (gpGame->players[hro->ownerIdx].personality == PERSONALITY_HUMAN)
								DoEvent_orig(cell, locX, locY);
				else
								DoAIEvent_orig(cell, hro, locX, locY);
				hero_extra->VisitArena(locX, locY, true);
}

void advManager::HandleAlchemistTower(class mapCell *cell, int locType, hero *hro, SAMPLE2 *res2, int locX, int locY)
{
				int cursedArtifacts = 0;
				for (int i = 0; i < MAX_ARTIFACTS; i++)
								if (IsCursedItem(hro->artifacts[i]))
												cursedArtifacts++;
				if (gpGame->players[hro->ownerIdx].personality != PERSONALITY_HUMAN)
				{
								int cost = 750 * cursedArtifacts;
								for (int i = 0; i < MAX_ARTIFACTS; i++)
												if (IsCursedItem(hro->artifacts[i]))
												{
																GiveTakeArtifactStat(hro, hro->artifacts[i], 1);
																hro->artifacts[i] = -1;
												}
								if (gpGame->players[hro->ownerIdx].resources[RESOURCE_GOLD] <= cost)
												gpGame->players[hro->ownerIdx].resources[RESOURCE_GOLD] = 0;
								else
												gpGame->players[hro->ownerIdx].resources[RESOURCE_GOLD] -= cost;
								return;
				}
				if (cursedArtifacts >= 1)
				{
								this->EventSound(locType, 0, res2);
								std::string str = "As you enter the Alchemist's Tower, a hobbled, graying man in a brown cloak makes his way towards you.  He checks your pack, and sees that you have ";
								str += std::to_string(cursedArtifacts);
								str += " cursed item";
								if (cursedArtifacts != 1)
												str += "s";
								str += ".  The alchemist will remove your cursed artifacts for 750 gold each (you can choose what to keep).  Do you pay?";
								bool res = H2QuestionBox(&str[0]);
								for (int i = 0; res && i < MAX_ARTIFACTS; i++)
												if (IsCursedItem(hro->artifacts[i]))
												{
																str = "Would you like to remove the ";
																str += GetArtifactName(hro->artifacts[i]);
																str += " for 750 gold";
																int answer = H2NormalDialog(&str[0], DIALOG_YES_NO, -1, -1, IMAGE_GROUP_ARTIFACTS, hro->artifacts[i], IMAGE_GOLD, 750, 0);
																if (answer == BUTTON_CODE_OKAY)
																				if (gpCurPlayer->resources[RESOURCE_GOLD] >= 750)
																				{
																								gpCurPlayer->resources[RESOURCE_GOLD] -= 750;
																								GiveTakeArtifactStat(hro, hro->artifacts[i], 1);
																								hro->artifacts[i] = -1;
																				}
																				else
																				{
																								str = "You hear a voice from behind the locked door, \"You don't have enough gold to pay for my services.\"";
																								H2NormalDialog(&str[0], DIALOG_OKAY, -1, -1, IMAGE_GOLD, 750, IMAGE_GOLD, gpCurPlayer->resources[RESOURCE_GOLD], 0);
																								break;
																				}
												}
				}
				else
				{
								char str[] = "You hear a voice from high above in the tower, \"Go away! I can't help you!\"";
								H2MessageBox(str);
				}
}

int advManager::MapPutArmy(int x, int y, int monIdx, int monQty) {
  int cellIdx = y * gpGame->map.height + x;
  mapCell * loc = &gpGame->map.tiles[cellIdx];
  loc->objectIndex = monIdx;
  loc->extraInfo = monQty;
  loc->objTileset = TILESET_MONSTER;
  loc->objType = TILE_HAS_EVENT | LOCATION_ARMY_CAMP;
  loc->overlayIndex = -1;
  loc->field_4_1 = 0;
  loc->isShadow = 0;
  return 0;
}

/*int advManager::MapPutResource(int x, int y, int resIdx, int resQty) {
	//gResourceNames[0] == "Wood"
	//gResourceNames[1] == "Mercury"
	//gResourceNames[2] == "Ore"
	//gResourceNames[3] == "Sulfur"
	//gResourceNames[4] == "Crystal"
	//gResourceNames[5] == "Gems"
	//gResourceNames[6] == "Gold"
	//resType == loc->objectIndex >> 1
	int cellIdx = y * gpGame->map.height + x;
	mapCell * loc = &gpGame->map.tiles[cellIdx];
	loc->objectIndex = resIdx;
	loc->extraInfo = resQty;
	loc->objTileset = TILESET_OBJECT_RESOURCE;
	loc->objType = TILE_HAS_EVENT | LOCATION_RESOURCE;
	loc->overlayIndex = -1;
	loc->field_4_1 = 0;
	loc->isShadow = 0;
	return 0;
}*/

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
										unsigned char visited = (PlayerVisitedObject[xLoc][yLoc] >> gpCurPlayer->color) & 1u;
										if (visited)
										{
														ShrineQuickInfo(xLoc, yLoc);
														return;
										}
						}
						else if (locationType == LOCATION_WITCH_HUT)
						{
										unsigned char visited = (PlayerVisitedObject[xLoc][yLoc] >> gpCurPlayer->color) & 1u;
										unsigned char visited2 = (PlayerVisitedObject[xLoc][yLoc + 1] >> gpCurPlayer->color) & 1u;
										//The Witch's Hut extends across 3 cells on the y-axis
										//We only check for the bottom 2 because it would create visual pollution
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
						else if (mapCell->objTileset == 61)
						{
										switch (mapCell->objectIndex)
										{
										case 40:
														ArenaQuickInfo(xLoc, yLoc, xLoc + 1, yLoc + 1);
														return;
										case 49:
														ArenaQuickInfo(xLoc, yLoc, xLoc, yLoc + 1);
														return;
										case 50:
														ArenaQuickInfo(xLoc, yLoc, xLoc - 1, yLoc + 1);
														return;
										case 69:
														ArenaQuickInfo(xLoc, yLoc, xLoc + 1, yLoc);
														return;
										case 70:
														ArenaQuickInfo(xLoc, yLoc, xLoc, yLoc);
														return;
										case 71:
														ArenaQuickInfo(xLoc, yLoc, xLoc - 1, yLoc);
														return;
										}
						}
						else if (mapCell->objectIndex == -1)
						{
										if (mapCell->overlayTileset == 61)
										{
														switch (mapCell->overlayIndex)
														{
														case 4:
																		ArenaQuickInfo(xLoc, yLoc, xLoc + 1, yLoc + 2);
																		return;
														case 13:
																		ArenaQuickInfo(xLoc, yLoc, xLoc, yLoc + 2);
																		return;
														case 22:
																		ArenaQuickInfo(xLoc, yLoc, xLoc - 1, yLoc + 2);
																		return;
														}
										}
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

void advManager::ArenaQuickInfo(int xLoc, int yLoc, int TriggerX, int TriggerY)
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
				std::string str = "Arena\n";
				if (gpCurPlayer->curHeroIdx != -1)
				{
								HeroExtraII* hero_extra = HeroExtras[gpCurPlayer->curHeroIdx];
								if (hero_extra->HasVisitedArena(TriggerX, TriggerY))
												str += "\n(already visited)";
								else
												str += "\n(not visited)";
				}
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

void __thiscall advManager::HouseEvent(class hero * hro, class mapCell * cell)
{
				if (gpGame->players[hro->ownerIdx].personality != PERSONALITY_HUMAN)
				{
								HouseEvent_orig(hro, cell);
								return;
				}
				int locationType = cell->objType & 0x7F;
				std::string askstring = "{%s}\n\nA group of " + std::to_string(cell->extraInfo) + " %s with a desire for greater glory wish to join you. Do you accept?";
				std::string emptystring = "{%s}\n\nAs you approach the dwelling, you notice that there is no one here.";
				char str[200];
				int creature;
				switch (locationType)
				{
				case LOCATION_ARCHERS_HOUSE:
								sprintf(str, askstring.c_str(), "Archer\'s House", "archers");
								askstring = str;
								sprintf(str, emptystring.c_str(), "Archer\'s House");
								emptystring = str;
								creature = CREATURE_ARCHER;
								break;
				case LOCATION_GOBLIN_HUT:
								sprintf(str, askstring.c_str(), "Goblin Hut", "goblins");
								askstring = str;
								sprintf(str, emptystring.c_str(), "Goblin Hut");
								emptystring = str;
								creature = CREATURE_GOBLIN;
								break;
				case LOCATION_DWARF_COTTAGE:
								sprintf(str, askstring.c_str(), "Dwarf Cottage", "dwarves");
								askstring = str;
								sprintf(str, emptystring.c_str(), "Dwarf Cottage");
								emptystring = str;
								creature = CREATURE_DWARF;
								break;
				case LOCATION_DWARF_CABIN:
								sprintf(str, askstring.c_str(), "Dwarf Cabin", "dwarves");
								askstring = str;
								sprintf(str, emptystring.c_str(), "Dwarf Cabin");
								emptystring = str;
								creature = CREATURE_DWARF;
								return;
				case LOCATION_PEASANT_HUT:
								sprintf(str, askstring.c_str(), "Peasant Hut", "peasants");
								askstring = str;
								sprintf(str, emptystring.c_str(), "Peasant Hut");
								emptystring = str;
								creature = CREATURE_PEASANT;
								break;
				case LOCATION_LOG_CABIN:
								HouseEvent_orig(hro, cell);
								return;
				case LOCATION_WATCH_TOWER:
								sprintf(str, askstring.c_str(), "Watch Tower", "orcs");
								askstring = str;
								sprintf(str, emptystring.c_str(), "Watch Tower");
								emptystring = str;
								creature = CREATURE_ORC;
								break;
				case LOCATION_TREE_HOUSE:
								sprintf(str, askstring.c_str(), "Tree House", "sprites");
								askstring = str;
								sprintf(str, emptystring.c_str(), "Tree House");
								emptystring = str;
								creature = CREATURE_SPRITE;
								break;
				case LOCATION_HALFLING_HOLE:
								sprintf(str, askstring.c_str(), "Halfling Hole", "halflings");
								askstring = str;
								sprintf(str, emptystring.c_str(), "Halfling Hole");
								emptystring = str;
								creature = CREATURE_HALFLING;
								break;
				case LOCATION_EXCAVATION:
								sprintf(str, askstring.c_str(), "Excavation", "skeletons");
								askstring = str;
								sprintf(str, emptystring.c_str(), "Excavation");
								emptystring = str;
								creature = CREATURE_SKELETON;
								break;
				case LOCATION_CAVE:
								sprintf(str, askstring.c_str(), "Cave", "centaurs");
								askstring = str;
								sprintf(str, emptystring.c_str(), "Cave");
								emptystring = str;
								creature = CREATURE_CENTAUR;
								break;
				}
				if (cell->extraInfo >= 1)
				{
								strcpy(str, askstring.c_str());
								this->EventWindow(-1, 2, str, IMAGE_GROUP_UNIT, creature, -1, 0, -1);
								if (gpWindowManager->buttonPressedCode == BUTTON_YES)
												if (hro->army.CanJoin(creature))
												{
																hro->army.Add(creature, cell->extraInfo, -1);
																cell->extraInfo = 0;
												}
												else
																this->EventWindow(-1, 1, "You are unable to recruit at this time, your ranks are full.", -1, 0, -1, 0, -1);
				}
				else
				{
								strcpy(str, emptystring.c_str());
								this->EventWindow(-1, 1, str, -1, 0, -1, 0, -1);
				}
}

void advManager::PlayerMonsterInteract(mapCell *cell, mapCell *other, hero *player, int *window, int a1, int a2, int a3, int a4, int a5) {
	int x;
	int y;
	if (cell->objType != (LOCATION_ARMY_CAMP | TILE_HAS_EVENT)) {
		this->PlayerMonsterInteract_orig(cell, other, player, window, a1, a2, a3, a4, a5);
		return;
	}
	if (GetMapCellXY(cell, &x, &y)) {
		ScriptCallback("OnMonsterInteract", x, y);
	}
	this->PlayerMonsterInteract_orig(cell, other, player, window, a1, a2, a3, a4, a5);
}

void advManager::ComputerMonsterInteract(mapCell *cell, hero *computer, int *a1) {
	int x;
	int y;
	if (cell->objType != (LOCATION_ARMY_CAMP | TILE_HAS_EVENT)) {
		this->ComputerMonsterInteract_orig(cell, computer, a1);
		return;
	}
	if (GetMapCellXY(cell, &x, &y)) {
		ScriptCallback("OnMonsterInteract", x, y);
	}
	this->ComputerMonsterInteract_orig(cell, computer, a1);
}

void advManager::UpdateTownLocators(int a2, int updateScreen) { 
  if(!gbThisNetHumanPlayer[giCurPlayer])
    return;

  const int NUM_GUI_ROWS = 4;
  for(int guiRow = 0; guiRow < NUM_GUI_ROWS; ++guiRow) {
    int townID = *(&gpCurPlayer->castlesOwned[guiRow] + gpCurPlayer->relatedToUnknown);
    tag_message evt;
    evt.eventCode = INPUT_GUI_MESSAGE_CODE;
    evt.xCoordOrKeycode = 8;
    evt.yCoordOrFieldID = guiRow + 32;
    if(gpCurPlayer->mightBeCurCastleIdx == -1 || gpCurPlayer->mightBeCurCastleIdx != townID || gbAllBlack)
      evt.payload = (void *)36;
    else
      evt.payload = (void *)153;
    this->adventureScreen->BroadcastMessage(evt);
    
    if(townID == -1 || gbAllBlack) {
      int guiField = guiRow + 16;
      GUISetImgIdx(this->adventureScreen, guiField, guiRow + 5);
      GUIRemoveFlag(this->adventureScreen, guiField, 2);
      guiField = guiRow + 300;
      GUIRemoveFlag(this->adventureScreen, guiField, 4);
    } else {
      int guiField = guiRow + 16;
      GUIAddFlag(this->adventureScreen, guiField, 2);

      int faction = gpGame->castles[townID].factionID;     
      if(!(gpGame->castles[townID].buildingsBuiltFlags & 0x40))
        GUISetImgIdx(this->adventureScreen, guiField, townIconFrames[faction]);
      else
        GUISetImgIdx(this->adventureScreen, guiField, castleIconFrames[faction]);

      guiField = guiRow + 300;
      if(BitTest((const LONG*)gpGame->builtToday, townID))
        GUIAddFlag(this->adventureScreen, guiField, 4);
      else
        GUIRemoveFlag(this->adventureScreen, guiField, 4);
    }
  }
  this->castleScrollbarKnob->offsetY = gpCurPlayer->numCastles >= 5 ? (unsigned __int16)(signed __int64)((double)gpCurPlayer->relatedToUnknown * 74.0 / (double)((signed int)gpCurPlayer->numCastles - 4) + 195.0) : 232;
  if(a2)
    this->adventureScreen->DrawWindow(updateScreen);
}

void advManager::TownQuickView(int townID, int _104, int xOff, int a3) {
  if(townID == -1)
    return;  
  
  const int MAX_GARRISON_CREATURE_SLOTS = 5;

  icon *res = gpResourceManager->GetIcon("mons32.icn");
  town *town = &gpGame->castles[townID];

  if(xOff == -1) {
    xOff = 328;
    a3 = 176;
  }

  heroWindow *window = new heroWindow(xOff, a3, "qtown1.bin");
  if(!window)
    MemError();

  int thievesGuildsLevel;
  if(town->ownerIdx != giCurPlayer && giDebugLevel < 2) {
    thievesGuildsLevel = gpGame->getNumberOfThievesGuilds(giCurPlayer);
    if(thievesGuildsLevel > 2)
      thievesGuildsLevel = 2;
  } else {
    thievesGuildsLevel = 3;
  }
  if(this->IsCrystalBallInEffect(town->x, town->y, 8))
    thievesGuildsLevel = 3;
  SetWinText(window, 19);
  
  int faction = town->factionID;
  if(!(gpGame->castles[townID].buildingsBuiltFlags & 0x40))
    GUISetImgIdx(window, 2, townIconFrames[faction]);
  else
    GUISetImgIdx(window, 2, castleIconFrames[faction]);

  // built today icon
  if(thievesGuildsLevel != 3 || !BitTest((const LONG*)gpGame->builtToday, town->idx))
    GUIRemoveFlag(window, 300, 4);

  if(town->ownerIdx == -1) {
    GUIRemoveFlag(window, 8, 4);
    GUIRemoveFlag(window, 9, 4);
  } else {
    int val = 2 * gpGame->players[town->ownerIdx].color;
    GUISetImgIdx(window, 8, val);
    GUISetImgIdx(window, 9, val + 1);
  }

  GUISetText(window, 1, GetTownName(town->idx));

  int garrisonCreatures = 0;
  for(int i = 0; i < MAX_GARRISON_CREATURE_SLOTS; ++i) {
    if(town->garrison.creatureTypes[i] != -1)
      ++garrisonCreatures;
  }

  if(thievesGuildsLevel && garrisonCreatures) {
    iconWidget *iconWidgetArray[MAX_GARRISON_CREATURE_SLOTS];
    textWidget *textWidgetArray[MAX_GARRISON_CREATURE_SLOTS];

    int yOffset = 76;
    int numCreaturesSecondRow;
    int numCreaturesFirstRow;
    if(garrisonCreatures < 1 || garrisonCreatures >= 5) {
      numCreaturesSecondRow = 2;
      numCreaturesFirstRow = 3;
    } else if(garrisonCreatures >= 1 && garrisonCreatures <= 3) {
      yOffset = 98;
      numCreaturesSecondRow = garrisonCreatures;
      numCreaturesFirstRow = 0;
    } else {
      numCreaturesSecondRow = 2;
      numCreaturesFirstRow = 2;
    }
    
    int xOffset = 0;
    int xOffset2 = (192 / numCreaturesSecondRow - 32) / 2 + 22;
    int xOffset3 = 192 / numCreaturesSecondRow;

    int widgetIdx = 0;
    int garrisonIdx = 0;
    for(int i = 0; i < numCreaturesSecondRow; ++i) {
      if(garrisonCreatures == 5) {
        if(i)
          xOffset = -12;
        else
          xOffset = 12;
      }
      while(town->garrison.creatureTypes[garrisonIdx] == -1)
        ++garrisonIdx;

      int imgIdx = town->garrison.creatureTypes[garrisonIdx];

      IconEntry *iconEntry = GetIconEntry(res, imgIdx);

      int v10 = xOffset + xOffset2 + xOffset3 * widgetIdx - iconEntry->offsetX;
      int x = (32 - iconEntry->width) / 2 + v10 + 1;
      int y = (yOffset - iconEntry->offsetY) - iconEntry->height + 30;

      iconWidgetArray[widgetIdx] = new iconWidget(x, y, 32, 32, "mons32.icn", imgIdx, 0, -1, 16, 1);
      if(!iconWidgetArray[widgetIdx])
        MemError();           

      x = xOffset + xOffset2 + xOffset3 * widgetIdx - 14;
      y = yOffset + 32;
      char *quantityStr = this->GetQuantityString(thievesGuildsLevel, town, garrisonIdx);
      textWidgetArray[widgetIdx] = new textWidget(x, y, 60, 12, quantityStr, "smalfont.fnt", 1, -1, 512, 1);
      if(!textWidgetArray[widgetIdx])
        MemError();

      window->AddWidget((widget *)iconWidgetArray[widgetIdx], -1);
      window->AddWidget((widget *)textWidgetArray[widgetIdx++], -1);
      ++garrisonIdx;
    }

    if(numCreaturesFirstRow) {
      xOffset3 = 192 / numCreaturesFirstRow;
      int v31 = (192 / numCreaturesFirstRow - 32) / 2 + 22;
      int yOffset2 = yOffset + 44;
      for(int i = numCreaturesSecondRow; numCreaturesFirstRow + numCreaturesSecondRow > i; ++i) {
        while(town->garrison.creatureTypes[garrisonIdx] == -1)
          ++garrisonIdx;
        int imgIdx = town->garrison.creatureTypes[garrisonIdx];

        int v14 = yOffset2 - GetIconEntry(res, imgIdx)->offsetY;
        int y = v14 - GetIconEntry(res, imgIdx)->height + 30;
        int v16 = v31 + xOffset3 * (widgetIdx - numCreaturesSecondRow) - GetIconEntry(res, imgIdx)->offsetX;
        IconEntry *v17 = GetIconEntry(res, imgIdx);
        int x = (32 - v17->width) / 2 + v16 + 1;
        iconWidgetArray[widgetIdx] = new iconWidget(x, y, 32, 32, "mons32.icn", imgIdx, 0, -1, 16, 1);

        if(!iconWidgetArray[widgetIdx])
          MemError();       

        x = v31 + xOffset3 * (widgetIdx - numCreaturesSecondRow) - 14;
        y = yOffset2 + 32;
        char *quantityStr = this->GetQuantityString(thievesGuildsLevel, town, garrisonIdx);
        textWidgetArray[widgetIdx] = new textWidget(x, y, 60, 12, quantityStr, "smalfont.fnt", 1, -1, 512, 1);

        if(!textWidgetArray[widgetIdx])
          MemError();
        window->AddWidget((widget *)iconWidgetArray[widgetIdx], -1);
        window->AddWidget((widget *)textWidgetArray[widgetIdx++], -1);
        ++garrisonIdx;
      }
    }
  } else {
    std::string str;
    if(thievesGuildsLevel)
      str = "None";
    else
      str = "Unknown";
    char *charStr = (char*)BaseAlloc(str.size(), __FILE__, __LINE__);
          charStr = strdup(str.c_str());

    widget *guiObj = new textWidget(13, 117, 211, 12, charStr, "smalfont.fnt", 1, -1, 512, 1);

    if(!guiObj)
      MemError();
    window->AddWidget(guiObj, -1);
  }
  gpWindowManager->AddWindow(window, -1, 1);
  int tmpViewX = this->viewX;
  int tmpViewY = this->viewY;
  this->viewX = town->x - 7;
  this->viewY = town->y - 7;
  this->UpdateRadar(1, 0);
  QuickViewWait();
  gpWindowManager->RemoveWindow(window);
  delete window;
  this->viewX = tmpViewX;
  this->viewY = tmpViewY;
  this->UpdateRadar(1, 0);
  this->CompleteDraw(0);
  this->UpdateScreen(0, 0);
  // Never called anyway?
  //if(evt.eventCode == 8 && town->ownerIdx == giCurPlayer)
  //  this->SetTownContext(town->idx);
  gpResourceManager->Dispose((resource *)res);
}

char * advManager::GetQuantityString(int thievesGuildsLevel, town* town, int garrisonIdx) {
  std::string quantityStr;
  int quantity = town->garrison.quantities[garrisonIdx];
  if(thievesGuildsLevel == 3)
    quantityStr = std::to_string(quantity);
  else if(thievesGuildsLevel == 2)
    quantityStr = this->GetArmySizeName(quantity, 0);
  else
    quantityStr = "???";

  char *charStr = (char*)BaseAlloc(quantityStr.size(), __FILE__, __LINE__);
  charStr = strdup(quantityStr.c_str());
  return charStr;
}

bool GetMapCellXY(mapCell* cell, int* x, int* y) {
	for (int i = 0; i < gpGame->map.width; i++) {
		for (int j = 0; j < gpGame->map.height; j++) {
			if (cell == (&(gpGame->map.tiles[j * gpGame->map.width])) + i) {
				// heroCell = &this->map.tiles[heroLocationY * this->map.width] + heroLocationX;
				*x = i;
				*y = j;
				return true;
			}
		}
	}
	return false;
}

void advManager::DrawCell(int x, int y, int cellCol, int cellRow, int cellDrawingPhaseFlags, int a6) {
  if(!a6 && !bShowIt)
    return;

  int drawX = 32 * cellCol;
  int drawY = 32 * cellRow;

  // Draw stone outline
  if(!gbAllBlack && (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT)) {
    int outlineIdx = -1;
    if(x == -1) {
      if(y == -1) {
        outlineIdx = 16;
      } else if(y == MAP_HEIGHT) {
        outlineIdx = 19;
      } else if(y >= 0 && y < MAP_HEIGHT) {
        outlineIdx = (y & 3) + 32;
      }
    } else if(x == MAP_WIDTH) {
      if(y == -1) {
        outlineIdx = 17;
      } else if(y == MAP_HEIGHT) {
        outlineIdx = 18;
      } else if(y >= 0 && y < MAP_HEIGHT) {
        outlineIdx = (y & 3) + 24;
      }
    } else if(y == -1) {
      if(x >= 0 && x < MAP_WIDTH)
        outlineIdx = (x & 3) + 20;
    } else if(y == MAP_HEIGHT && x >= 0 && x < MAP_WIDTH) {
      outlineIdx = (x & 3) + 28;
    }
    if(outlineIdx == -1)
      outlineIdx = (((unsigned __int64)(x + 16) >> 32) ^ abs(x + 16) & 3)
      + 4 * ((((unsigned __int64)(y + 16) >> 32) ^ abs(y + 16) & 3) - ((unsigned __int64)(y + 16) >> 32))
      - ((unsigned __int64)(x + 16) >> 32);
    TileToBitmap(this->stonTileset, outlineIdx, gpWindowManager->screenBuffer, drawX, drawY);
    return;
  }


  bool unknownFlag;
  bool unknownFlag2;
  int unknownTerraIncognita;
  // Draw Terra Incognita
  if(!gbAllBlack && (mapRevealed[x + MAP_WIDTH * y] & giCurWatchPlayerBit) || gbDrawingPuzzle) {
    unknownFlag2 = false;
  } else {
    unknownFlag2 = true;
    if(gbAllBlack)
      unknownTerraIncognita = 0;
    else
      unknownTerraIncognita = advManager::GetCloudLookup(x, y);
    if(!unknownTerraIncognita) {
      if(cellDrawingPhaseFlags & 0x20)
        TileToBitmap(this->clofTileset, (x + y) & 3, gpWindowManager->screenBuffer, drawX, drawY);
      return;
    }
    if(unknownTerraIncognita < 100) {
      unknownFlag = false;
    } else {
      unknownFlag = true;
      unknownTerraIncognita -= 100;
    }
    if((unknownTerraIncognita == 1 || unknownTerraIncognita == 5) && x & 1)
      ++unknownTerraIncognita;
    if(unknownTerraIncognita == 3 && y & 1)
      ++unknownTerraIncognita;
  }

  // Drawing everything else
  int curHeroColor;
  int curHeroFaction;
  mapCell *curTile = this->GetCell(x, y);
  mapCellExtra *curExtra;
  icon* const tileIcon = this->tilesetIcns[curTile->objTileset];
  unsigned int const extraIdx = curTile->extraIdx;
  unsigned char const objectIndex = curTile->objectIndex;
  if(!(cellDrawingPhaseFlags & 0x20) || gbDrawingPuzzle) {
    if(cellDrawingPhaseFlags & 1) {
      // If you need actual code (instead of default behaviour) that was here before check commit 7bbe66a516d79f6aab874d401b0ca65226b851a3
      // For some reason it crashed Release build, so it was removed
      DrawCell_orig(x, y, cellCol, cellRow, cellDrawingPhaseFlags, a6);
      return;
    }
    if(cellDrawingPhaseFlags & 2) {
      // Drawing treasures / resources / main tiles of those
      if(objectIndex != 255 && !curTile->field_4_1 && !curTile->isShadow && !curTile->field_4_3 && curTile->objTileset != TILESET_MONSTER && (!gbDrawingPuzzle || bPuzzleDraw[curTile->objTileset])) {
        IconToBitmap(tileIcon, gpWindowManager->screenBuffer, drawX, drawY, objectIndex, 0, 0, 0, 480, 480, 0);
        if(curTile->hasObject) {
          int someOffset = GetIconEntry(tileIcon, objectIndex)->someSortOfLength & 0x1F;
          int idxOffset = this->field_202 % someOffset;
          if(curTile->objType == (TILE_HAS_EVENT | LOCATION_MAGIC_GARDEN)) {
            if(curTile->extraInfo)
              idxOffset = this->field_202 % (someOffset - 1);
            else
              idxOffset = someOffset - 1;
          }
          IconToBitmap(tileIcon, gpWindowManager->screenBuffer, drawX, drawY, idxOffset + objectIndex + 1, 0, 0, 0, 480, 480, 0);
        }
      }

      if(extraIdx && (unsigned char)this->map->cellExtras[extraIdx].objectIndex != 255)
        curExtra = &this->map->cellExtras[extraIdx];
      else
        curExtra = nullptr;
      while(curExtra) {
        if(!curExtra->field_4_1 && !curExtra->field_4_2 && !curExtra->field_4_3 && curExtra->objTileset != TILESET_MONSTER && (!gbDrawingPuzzle || bPuzzleDraw[curExtra->objTileset])) {
          IconToBitmap(this->tilesetIcns[curExtra->objTileset], gpWindowManager->screenBuffer, drawX, drawY, (unsigned char)curExtra->objectIndex, 0, 0, 0, 480, 480, 0);
          if(curExtra->animatedObject) {
            int someOffset = GetIconEntry(this->tilesetIcns[curExtra->objTileset], (unsigned char)curExtra->objectIndex)->someSortOfLength & 0x1F;
            int spriteIdx = (unsigned char)curExtra->objectIndex + this->field_202 % someOffset + 1;
            IconToBitmap(this->tilesetIcns[curExtra->objTileset], gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 0, 0, 0, 480, 480, 0);
          }
        }
        if((unsigned char)curExtra->nextIdx && (unsigned char)this->map->cellExtras[(unsigned short)curExtra->nextIdx].objectIndex != 255)
          curExtra = &this->map->cellExtras[(unsigned short)curExtra->nextIdx];
        else
          curExtra = nullptr;
      }
    }
    if((cellDrawingPhaseFlags & 8 || cellDrawingPhaseFlags & 0x80) && !gbDrawingPuzzle) {
      bool isDrawingHeroOrBoat = false;
      hero *curDrawingHero = nullptr;
      // Drawing creatures
      if(cellDrawingPhaseFlags & 8) {
        if(x > 0) {
          mapCell* creatureTile = this->GetCell(x - 1, y);
          if(creatureTile->objType == (TILE_HAS_EVENT | LOCATION_MINE)) {
            mine* currentMine = &gpGame->mines[creatureTile->extraInfo];
            if(currentMine->guardianType == CREATURE_GHOST) {
              IconToBitmap(this->tilesetIcns[10], gpWindowManager->screenBuffer, drawX - 16, drawY, (x + y + this->field_202) % 15, 1, 0, 0, 480, 480, 0);
            } else if(currentMine->guardianType != -1) {
              IconToBitmap(this->tilesetIcns[39], gpWindowManager->screenBuffer, drawX - 32, drawY, currentMine->guardianType - 62, 1, 0, 0, 480, 480, 0);
            }
          }
        }
        if(curTile->objTileset == TILESET_MONSTER) {
          if(this->field_2B2 != x || this->field_2B6 != y) {
            IconToBitmap(this->tilesetIcns[20], gpWindowManager->screenBuffer, drawX + 16, drawY + 30, 9 * objectIndex, 1, 0, 0, 480, 480, 0);
            int frameOffset;
            if(objectIndex != 59 && objectIndex != 60)
              frameOffset = (unsigned __int8)monAnimDrawFrame[*(&this->field_20A + (x & 3))];
            else
              frameOffset = *(&this->field_20A + (x & 3)) % 6;
            IconToBitmap(this->tilesetIcns[20], gpWindowManager->screenBuffer, drawX + 16, drawY + 30, frameOffset + 9 * objectIndex + 1, 1, 0, 0, 480, 480, 0);
          } else {
            IconToBitmap(this->tilesetIcns[20], gpWindowManager->screenBuffer, drawX + 16, drawY + 30, 8 - (this->field_2BA < 1u) + 9 * objectIndex, 1, 0, 0, 480, 480, 0);
          }
        }
      }

      int heroBoatYOffset;
      int heroBoatSpriteIdx;
      // Drawing boats and heroes
      if(curTile->objType == (TILE_HAS_EVENT | LOCATION_BOAT)) {
        curHeroColor = -1;
        curHeroFaction = FACTION_MULTIPLE;
        heroBoatSpriteIdx = this->GetCursorBaseFrame(gpGame->boats[curTile->extraInfo].field_3);
        isDrawingHeroOrBoat = true;
        heroBoatYOffset = -10;
      } else {
        heroBoatYOffset = 0;
        if(curTile->objType == (TILE_HAS_EVENT | LOCATION_HERO)) {
          curDrawingHero = &gpGame->heroes[curTile->extraInfo];
          curHeroColor = gpGame->players[curDrawingHero->ownerIdx].color;
          curHeroFaction = curDrawingHero->flags & HERO_AT_SEA ? 6 : curDrawingHero->factionID;
          heroBoatSpriteIdx = advManager::GetCursorBaseFrame(curDrawingHero->directionFacing);
          isDrawingHeroOrBoat = true;
          if(curDrawingHero->flags & 0x80)
            heroBoatYOffset = -10;
        }
      }

      if(isDrawingHeroOrBoat) {
        if(heroBoatSpriteIdx & 0x80) {
          if(cellDrawingPhaseFlags & 0x80) {
            if(this->field_276 && curHeroFaction != FACTION_MULTIPLE) {
              signed int idxOffset;
              int idxOffset2 = heroBoatSpriteIdx & 0x7F;
              if(idxOffset2 == 51)
                idxOffset2 = 56;
              if(idxOffset2 == 50)
                idxOffset2 = 57;
              if(idxOffset2 == 49)
                idxOffset2 = 58;
              if(idxOffset2 == 47)
                idxOffset2 = 55;
              if(idxOffset2 == 46)
                idxOffset2 = 55;
              if(idxOffset2 < 9 || idxOffset2 >= 36)
                idxOffset = 0;
              else
                idxOffset = 50;
              IconToBitmap(this->shadowIcon, gpWindowManager->screenBuffer, drawX, drawY + 31, idxOffset + idxOffset2, 1, 0, 0, 480, 480, 0);
            }
            if(this->field_276 && curHeroFaction == FACTION_MULTIPLE) {
              int xOffset;
              int xOffset2 = heroBoatSpriteIdx & 0x7F;
              if(xOffset2 < 9 || xOffset2 >= 36)
                xOffset = 0;
              else
                xOffset = 36;
              IconToBitmap(this->boatShadowIcon, gpWindowManager->screenBuffer, drawX, heroBoatYOffset + drawY + 31, xOffset + xOffset2, 1, 0, 0, 480, 480, 0);
            }
          } else {
            if(curHeroFaction == FACTION_MULTIPLE && !(curTile->flags & 4))
              FlipIconToBitmap(this->frothIcon, gpWindowManager->screenBuffer, drawX + 32, heroBoatYOffset + drawY + 31, heroBoatSpriteIdx & 0x7F, 1, 0, 0, 480, 480, 0);
            FlipIconToBitmap(ironfistHeroMapIcons[curHeroFaction], gpWindowManager->screenBuffer, drawX + 32, heroBoatYOffset + drawY + 31, heroBoatSpriteIdx & 0x7F, 1, 0, 0, 480, 480, 0);
            if(curHeroColor != -1) {
              if(curHeroFaction == FACTION_MULTIPLE)
                FlipIconToBitmap(this->flagIconsBoat[curHeroColor], gpWindowManager->screenBuffer, drawX + 32, heroBoatYOffset + drawY + 31, heroBoatSpriteIdx & 0x7F, 1, 0, 0, 480, 480, 0);
              else {
                int spriteIdx = (heroBoatSpriteIdx & 0x7F) + (((unsigned __int64)this->field_202 >> 32) ^ abs(this->field_202) & 7) - ((unsigned __int64)this->field_202 >> 32) + 56;
                FlipIconToBitmap(this->flagIconsHero[curHeroColor], gpWindowManager->screenBuffer, drawX + 32, drawY + 31, spriteIdx, 1, 0, 0, 480, 480, 0);
              }
            }
          }
        } else if(cellDrawingPhaseFlags & 0x80) {
          if(this->field_276 && curHeroFaction != FACTION_MULTIPLE && cellDrawingPhaseFlags & 0x80)
            IconToBitmap(this->shadowIcon, gpWindowManager->screenBuffer, drawX, drawY + 31, heroBoatSpriteIdx, 1, 0, 0, 480, 480, 0);
          if(this->field_276 && curHeroFaction == FACTION_MULTIPLE)
            IconToBitmap(this->boatShadowIcon, gpWindowManager->screenBuffer, drawX, heroBoatYOffset + drawY + 31, heroBoatSpriteIdx, 1, 0, 0, 480, 480, 0);
        } else {
          if(curHeroFaction == FACTION_MULTIPLE && !(curTile->flags & 4))
            IconToBitmap(this->frothIcon, gpWindowManager->screenBuffer, drawX, heroBoatYOffset + drawY + 31, heroBoatSpriteIdx, 1, 0, 0, 480, 480, 0);
          IconToBitmap(ironfistHeroMapIcons[curHeroFaction], gpWindowManager->screenBuffer, drawX, heroBoatYOffset + drawY + 31, heroBoatSpriteIdx, 1, 0, 0, 480, 480, 0);
          if(curHeroColor != -1) {
            if(curHeroFaction == FACTION_MULTIPLE)
              IconToBitmap(this->flagIconsBoat[curHeroColor], gpWindowManager->screenBuffer, drawX, heroBoatYOffset + drawY + 31, heroBoatSpriteIdx & 0x7F, 1, 0, 0, 480, 480, 0);
            else {
              int spriteIdx = (heroBoatSpriteIdx & 0x7F) + (((unsigned __int64)this->field_202 >> 32) ^ abs(this->field_202) & 7) - ((unsigned __int64)this->field_202 >> 32) + 56;
              IconToBitmap(this->flagIconsHero[curHeroColor], gpWindowManager->screenBuffer, drawX, drawY + 31, spriteIdx, 1, 0, 0, 480, 480, 0);
            }
          }
        }
      }
      if(this->field_272 && curTile->flags & MAP_CELL_HAS_ACTIVE_HERO && (!this->hasDrawnCursor || cellDrawingPhaseFlags & 0x80) && this->viewX + 7 == x && this->viewY + 7 == y && !(cellDrawingPhaseFlags & 0x80)) {
        this->DrawCursorShadow();
        this->DrawCursor();
        this->hasDrawnCursor = 1;
      }
    }
    if(cellDrawingPhaseFlags & 4 || cellDrawingPhaseFlags & 0x40) {
      if(cellDrawingPhaseFlags & 4 && objectIndex != 255) {
        if(curTile->field_4_3 && (!gbDrawingPuzzle || bPuzzleDraw[curTile->objTileset])) {
          IconToBitmap(tileIcon, gpWindowManager->screenBuffer, drawX, drawY, objectIndex, 0, 0, 0, 480, 480, 0);
          if(curTile->hasObject) {
            int someOffset = GetIconEntry(tileIcon, objectIndex)->someSortOfLength & 0x1F;
            int spriteIdx = objectIndex + this->field_202 % someOffset + 1;
            IconToBitmap(tileIcon, gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 0, 0, 0, 480, 480, 0);
          }
        }

        if(extraIdx && (unsigned char)this->map->cellExtras[extraIdx].objectIndex != 255)
          curExtra = &this->map->cellExtras[extraIdx];
        else
          curExtra = nullptr;
        while(curExtra) {
          if(curExtra->field_4_3 && (!gbDrawingPuzzle || bPuzzleDraw[curExtra->objTileset])) {
            IconToBitmap(this->tilesetIcns[curExtra->objTileset], gpWindowManager->screenBuffer, drawX, drawY, (unsigned char)curExtra->objectIndex, 0, 0, 0, 480, 480, 0);
            if(curExtra->animatedObject) {
              int someOffset = GetIconEntry(this->tilesetIcns[curExtra->objTileset], (unsigned char)curExtra->objectIndex)->someSortOfLength & 0x1F;
              int spriteIdx = (unsigned char)curExtra->objectIndex + this->field_202 % someOffset + 1;
              IconToBitmap(this->tilesetIcns[curExtra->objTileset], gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 0, 0, 0, 480, 480, 0);
            }
          }
          if((unsigned char)curExtra->nextIdx && (unsigned char)this->map->cellExtras[(unsigned short)curExtra->nextIdx].objectIndex != 255)
            curExtra = &this->map->cellExtras[(unsigned short)curExtra->nextIdx];
          else
            curExtra = nullptr;
        }
      }
      if((unsigned char)curTile->overlayIndex != 255
        && (cellDrawingPhaseFlags & 4 && !curTile->hasLateOverlay || cellDrawingPhaseFlags & 0x40 && curTile->hasLateOverlay)) {
        if(!gbDrawingPuzzle || bPuzzleDraw[curTile->overlayTileset]) {
          int spriteIdx = (unsigned char)curTile->overlayIndex;
          IconToBitmap(this->tilesetIcns[curTile->overlayTileset], gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, curTile->overlayTileset == TILESET_FLAG, 0, 0, 480, 480, 0);
          if(curTile->hasOverlay) {
            int someOffset = GetIconEntry(this->tilesetIcns[curTile->overlayTileset], (unsigned char)curTile->overlayIndex)->someSortOfLength & 0x1F;
            spriteIdx = (unsigned char)curTile->overlayIndex + this->field_202 % someOffset + 1;
            IconToBitmap(this->tilesetIcns[curTile->overlayTileset], gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 0, 0, 0, 480, 480, 0);
          }
        }
      }

      if(extraIdx && (unsigned char)this->map->cellExtras[extraIdx].overlayIndex != 255)
        curExtra = &this->map->cellExtras[extraIdx];
      else
        curExtra = nullptr;
      while(curExtra) {
        if(cellDrawingPhaseFlags & 4 && !curExtra->hasLateOverlay || cellDrawingPhaseFlags & 0x40 && curExtra->hasLateOverlay) {
          if(!gbDrawingPuzzle || bPuzzleDraw[curExtra->tileset]) {
            int spriteIdx = (unsigned char)curExtra->overlayIndex;
            IconToBitmap(this->tilesetIcns[curExtra->tileset], gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, curExtra->tileset == TILESET_FLAG, 0, 0, 480, 480, 0);
            if(curExtra->animatedLateOverlay) {
              int someOffset = GetIconEntry(this->tilesetIcns[curExtra->tileset], curExtra->overlayIndex)->someSortOfLength & 0x1F;
              spriteIdx = (unsigned char)curExtra->overlayIndex + this->field_202 % someOffset + 1;
              IconToBitmap(this->tilesetIcns[curExtra->tileset], gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 0, 0, 0, 480, 480, 0);
            }
          }
        }
        if((unsigned char)curExtra->nextIdx && (unsigned char)this->map->cellExtras[(unsigned short)curExtra->nextIdx].overlayIndex != 255)
          curExtra = &this->map->cellExtras[(unsigned short)curExtra->nextIdx];
        else
          curExtra = nullptr;
      }
    }
  } else if(unknownFlag2) { // Draw Terra Incognita borders
    if(unknownFlag)
      FlipIconToBitmap(this->clopIcon, gpWindowManager->screenBuffer, drawX + 31, drawY, unknownTerraIncognita - 1, 0, 0, 0, 0, 0, 0);
    else
      IconToBitmap(this->clopIcon, gpWindowManager->screenBuffer, drawX, drawY, unknownTerraIncognita - 1, 0, 0, 0, 0, 0, 0);
  } else if(this->field_A2 && *(unsigned short *)(2 * x + 2 * MAP_WIDTH * y + this->sizeOfSomethingMapRelated)) {
    if((*(unsigned short *)(2 * x + 2 * MAP_WIDTH * y + this->sizeOfSomethingMapRelated) >> 8) & 1)
      IconToBitmapColorTable(this->tilesetIcns[17], gpWindowManager->screenBuffer, drawX - 12, drawY + 2,
      (unsigned __int8)(*(unsigned short *)(2 * x + 2 * MAP_WIDTH * y + this->sizeOfSomethingMapRelated) - 1),
        1, 0, 0, 480, 480, 0, gColorTableRed, 1);
    else
      IconToBitmap(this->tilesetIcns[17], gpWindowManager->screenBuffer, drawX - 12, drawY + 2,
      (unsigned __int8)(*(unsigned short *)(2 * x + 2 * MAP_WIDTH * y + this->sizeOfSomethingMapRelated) - 1),
        1, 0, 0, 480, 480, 0);
  }

}

void advManager::DrawCursor() {
  if(!bShowIt || bSpecialHideCursor)
    return;

  if(gbDrawSavedCursor) {
    this->mobilizedHeroDirection = S1cursorDirection;
    this->mobilizedHeroBaseFrameBit8IsFlip = S1cursorBaseFrame;
    this->mobilizedHeroAnimPos = S1cursorFrameCount;
    this->mobilizedHeroCycle = S1cursorCycle;
    this->mobilizedHeroTurning = S1cursorTurning;
  }

  int drawX = this->mapPortLeftX + 224;
  int drawY = this->mapPortTopY + 255;
  if(this->mobilizedHeroFactionOrBoat == FACTION_MULTIPLE)
    drawY = this->mapPortTopY + 245;
  if(this->mobilizedHeroBaseFrameBit8IsFlip & 0x80) {
    drawX = this->mapPortLeftX + 256;
    int spriteIdx = this->mobilizedHeroAnimPos + (this->mobilizedHeroBaseFrameBit8IsFlip & 0x7F);
    if(this->mobilizedHeroFactionOrBoat == FACTION_MULTIPLE && !(this->GetCell(this->viewX + 7, this->viewY + 7)->flags & 4))
      FlipIconToBitmap(this->frothIcon, gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 1, 0, 0, 480, 480, 0);
    FlipIconToBitmap(ironfistHeroMapIcons[this->mobilizedHeroFactionOrBoat], gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 1, 0, 0, 480, 480, 0);
    if(this->mobilizedHeroFactionOrBoat == FACTION_MULTIPLE) {
      FlipIconToBitmap(this->flagIconsBoat[gpCurPlayer->color], gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 0, 0, 0, 0, 0, 0);
    } else {
      if(!this->mobilizedHeroCycle)
        spriteIdx = (((unsigned __int64)this->field_202 >> 32) ^ abs(this->field_202) & 7)
        + (this->mobilizedHeroBaseFrameBit8IsFlip & 0x7F)
        - ((unsigned __int64)this->field_202 >> 32)
        + 56;
      FlipIconToBitmap(this->flagIconsHero[gpCurPlayer->color], gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 1, 0, 0, 480, 480, 0);
      ++this->field_206;
    }
  } else {
    int spriteIdx = this->mobilizedHeroAnimPos + this->mobilizedHeroBaseFrameBit8IsFlip;
    if(this->mobilizedHeroFactionOrBoat == FACTION_MULTIPLE && !(this->GetCell(this->viewX + 7, this->viewY + 7)->flags & 4))
      IconToBitmap(this->frothIcon, gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 1, 0, 0, 480, 480, 0);
    IconToBitmap(ironfistHeroMapIcons[this->mobilizedHeroFactionOrBoat], gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 1, 0, 0, 480, 480, 0);
    if(this->mobilizedHeroFactionOrBoat == FACTION_MULTIPLE) {
      IconToBitmap(this->flagIconsBoat[gpCurPlayer->color], gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 0, 0, 0, 0, 0, 0);
    } else {
      if(!this->mobilizedHeroCycle)
        spriteIdx = (((unsigned __int64)this->field_202 >> 32) ^ abs(this->field_202) & 7)
        + (this->mobilizedHeroBaseFrameBit8IsFlip & 0x7F)
        - ((unsigned __int64)this->field_202 >> 32)
        + 56;
      IconToBitmap(this->flagIconsHero[gpCurPlayer->color], gpWindowManager->screenBuffer, drawX, drawY, spriteIdx, 1, 0, 0, 480, 480, 0);
      ++this->field_206;
    }
  }

  if(this->mobilizedHeroCycle && gConfig.data[gbThisNetHumanPlayer[giCurPlayer]] != 4) {
    ++this->mobilizedHeroAnimPos;
    if(gConfig.data[gbThisNetHumanPlayer[giCurPlayer]] == 3 && (this->mobilizedHeroAnimPos == 4 || this->mobilizedHeroAnimPos == 1))
      ++this->mobilizedHeroAnimPos;
    if(!gConfig.data[gbThisNetHumanPlayer[giCurPlayer]]) {
      EveryOther = 1 - EveryOther;
      if(EveryOther)
        --this->mobilizedHeroAnimPos;
    }
  }

  if(this->mobilizedHeroAnimPos >= 8)
    this->mobilizedHeroAnimPos = 0;

  if(!this->mobilizedHeroTurning) {
    if(!this->mobilizedHeroAnimPos)
      hOldWalkSample = hNewWalkSample;
    if(this->mobilizedHeroAnimPos == 3 || gConfig.data[gbThisNetHumanPlayer[giCurPlayer]] == 4 && !bMoveSoundMade) {
      bMoveSoundMade = 1;
      if(!EveryOther) {
        int cell = this->GetCell(this->viewX + 7, this->viewY + 7)->groundIndex;
        hNewWalkSample = gpSoundManager->MemorySample(this->walkSamples[giGroundToTerrain[cell]]);
      }
    }
  }

  if(!gbDrawSavedCursor) {
    S1cursorDirection = this->mobilizedHeroDirection;
    S1cursorBaseFrame = this->mobilizedHeroBaseFrameBit8IsFlip;
    S1cursorFrameCount = this->mobilizedHeroAnimPos;
    S1cursorCycle = this->mobilizedHeroCycle;
    S1cursorTurning = this->mobilizedHeroTurning;
  }
}

void __fastcall GiveTakeArtifactStat(hero *h, int art, int take) {
  GiveTakeArtifactStat_orig(h, art, take);
  if(!take)
    ScriptCallback("OnArtifactGive", deepbind<hero*>(h), art);
  else
    ScriptCallback("OnArtifactTake", deepbind<hero*>(h), art);
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
int GetWagonType(int x, int y)
{
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if (cell->objType & 0x7F != LOCATION_WAGON)
								return -1;
				if (cell->extraInfo == 0)
								return 0;
				else if (cell->extraInfo & 0x80)
								return 2;
				else
								return 1;
}

bool isDwelling(int x, int y, int* monsterType)
{
				mapCell* cell = gpAdvManager->GetCell(x, y);
				int locationType = cell->objType & 0x7F;
				int dummy = -1;
				if (monsterType == nullptr)
								monsterType = &dummy; //if we get NULL then we don't return anything
				switch (locationType)
				{
				case LOCATION_ANCIENT_LAMP:
								*monsterType = CREATURE_GENIE;
								break;
				case LOCATION_ARCHERS_HOUSE:
								*monsterType = CREATURE_ARCHER;
								break;
				case LOCATION_GOBLIN_HUT:
								*monsterType = CREATURE_GOBLIN;
								break;
				case LOCATION_DWARF_COTTAGE:
								*monsterType = CREATURE_DWARF;
								break;
				case LOCATION_PEASANT_HUT:
								*monsterType = CREATURE_PEASANT;
								break;
				case LOCATION_LOG_CABIN:
								*monsterType = CREATURE_DWARF;
								break;
				case LOCATION_DRAGON_CITY:
								*monsterType = CREATURE_RED_DRAGON;
								break;
				case LOCATION_DESERT_TENT:
								*monsterType = CREATURE_NOMAD;
								break;
				case LOCATION_WAGON_CAMP:
								*monsterType = CREATURE_ROGUE;
								break;
				case LOCATION_WATCH_TOWER:
								*monsterType = CREATURE_ORC;
								break;
				case LOCATION_TREE_HOUSE:
								*monsterType = CREATURE_SPRITE;
								break;
				case LOCATION_TREE_CITY:
								*monsterType = CREATURE_SPRITE;
								break;
				case LOCATION_RUINS:
								*monsterType = CREATURE_MEDUSA;
								break;
				case LOCATION_DWARF_CABIN:
								*monsterType = CREATURE_DWARF;
								break;
				case LOCATION_HALFLING_HOLE:
								*monsterType = CREATURE_HALFLING;
								break;
				case LOCATION_CITY_OF_DEAD:
								*monsterType = CREATURE_POWER_LICH;
								break;
				case LOCATION_EXCAVATION:
								*monsterType = CREATURE_SKELETON;
								break;
				case LOCATION_TROLL_BRIDGE:
								*monsterType = CREATURE_TROLL;
								break;
				case LOCATION_CAVE:
								*monsterType = CREATURE_CENTAUR;
								break;
				case LOCATION_EXPANSION_DWELLING:
								switch (cell->extraInfo % 8)
								{
								case 0:
												*monsterType = CREATURE_GHOST;
												break;
								case 1:
												*monsterType = CREATURE_EARTH_ELEMENTAL;
												break;
								case 2:
												*monsterType = CREATURE_AIR_ELEMENTAL;
												break;
								case 3:
												*monsterType = CREATURE_FIRE_ELEMENTAL;
												break;
								case 4:
												*monsterType = CREATURE_WATER_ELEMENTAL;
												break;
								}
				default:
								return false;
				}
				return true;
}