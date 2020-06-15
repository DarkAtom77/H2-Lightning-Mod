extern "C" {
#include "lua/src/lua.h"
#include "lua/src/lualib.h"
#include "lua/src/lauxlib.h"
}

#include "adventure/adv.h"
#include "adventure/map.h"
#include "campaign/campaign.h"
#include "combat/combat.h"
#include "game/game.h"
#include "gui/dialog.h"
#include "town/town.h"
#include "artifacts.h"

#include "scripting/deepbinding.h"
#include "scripting/register.h"

#include "sound/sound.h"
#include <cmath>

enum SoundEffectWait { SND_DO_WAIT, SND_DONT_WAIT };

static bool PlaySoundEffect(std::string snd, SoundEffectWait wait, SAMPLE2* samp) {
	SAMPLE2 res = NULL_SAMPLE2;
	if (!snd.empty()) {
		char* src;
		snd += ".82m";
		src = strdup(snd.c_str());
		res = LoadPlaySample(src);
		if (samp != NULL) {
			memcpy(samp, &res, sizeof(SAMPLE2));
		}
		if (wait == SND_DO_WAIT) {
			WaitEndSample(res, res.sample);
		}
		free(src);
		return true;
	}
	return false;
}

static bool CheckLocationItem(mapCell *loc) {
	if (!(loc->objType&TILE_HAS_EVENT)) {
		return false;
	}
	switch (loc->objType^TILE_HAS_EVENT) {
		case LOCATION_ANCIENT_LAMP:
		case LOCATION_ARTIFACT:
		case LOCATION_RESOURCE:
		case LOCATION_CAMPFIRE:
		case LOCATION_TREASURE_CHEST:
		case LOCATION_SHIPWRECK_SURVIVOR:
		case LOCATION_FLOTSAM:
		case LOCATION_SEA_CHEST:
			return true;
	}
	return false;
}

static bool CheckBoolean(lua_State *L, int pos) {
	if (lua_isboolean(L, pos)) {
		return lua_toboolean(L, pos);
	}
	const char *msg = lua_pushfstring(L, "%s expected, got %s", lua_typename(L, LUA_TBOOLEAN), luaL_typename(L, pos));
	luaL_argerror(L, pos, msg);
	return false;
}

static int StackIndexOfArg(int argNumber, int numArgs) {
	return (numArgs - (argNumber - 1));
}

/************************************************ Dialogs ********************************************************/

static int l_msgBox(lua_State *L) {
  const char* msg = luaL_checkstring(L, 1);
  H2MessageBox((char*)msg);
  return 0;
}

extern int gbFunctionComplete;

static int l_AdvancedMessageBox(lua_State *L) {
  const char* msg = luaL_checkstring(L, 1);
  int dialogType = luaL_checkinteger(L, 2);
  int horizontal = luaL_checkinteger(L, 3);
  int vertical = luaL_checknumber(L, 4);
  int img1type = luaL_checknumber(L, 5);
  int img1arg = luaL_checknumber(L, 6);
  int img2type = luaL_checknumber(L, 7);
  int img2arg = luaL_checknumber(L, 8);
  int writeOr = luaL_checknumber(L, 9);
  int a10 = luaL_checknumber(L, 10);
  
  std::string msgCopy = msg;
  NormalDialog(&msgCopy[0], dialogType, horizontal, vertical, img1type, img1arg, img2type, img2arg, writeOr, a10);
 
  if (dialogType == DIALOG_LEARN_CHOICE) { // learn dialog
    lua_pushboolean(L, gpWindowManager->buttonPressedCode == BUTTON_CODE_LEARN_LEFT);
  } else if (dialogType == DIALOG_CANCEL_ALT) {
    lua_pushboolean(L, gbFunctionComplete);
  } else if (dialogType == DIALOG_YES_NO) {
    lua_pushboolean(L, gpWindowManager->buttonPressedCode == BUTTON_CODE_OKAY);
  } else {
    lua_pushboolean(L, true);
  }
  
  return 1;
}

static int l_questionBox(lua_State *L) {
  char* qst = (char*)luaL_checkstring(L, 1);
  lua_pushboolean(L, H2QuestionBox(qst));
  return 1;
}

static int l_inputBox(lua_State *L) {
  char* qst = (char*)luaL_checkstring(L, 1);
  int len = (int)luaL_checknumber(L, 2);
  char* input = H2InputBox(qst, len);
  lua_pushstring(L, input);
  FREE(input); // pushstring copies it
  return 1;
}

static int l_recruitBox(lua_State *L)
{
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
  int creature = (int)luaL_checknumber(L, 2);
  short quantity = (short)luaL_checknumber(L, 3);
  short startQ = quantity;
  gpAdvManager->ExpansionRecruitEvent(hro, creature, &quantity);

  lua_pushinteger(L, startQ - quantity);
  return 1;
} 

static void register_dialog_funcs(lua_State *L) {
  lua_register(L, "MessageBox", l_msgBox);
  lua_register(L, "AdvancedMessageBox", l_AdvancedMessageBox);
  lua_register(L, "QuestionBox", l_questionBox);
  lua_register(L, "InputBox", l_inputBox);
  lua_register(L, "RecruitBox", l_recruitBox);
}

/************************************************ Date ********************************************************/

static int l_getDay(lua_State *L) {
  lua_pushinteger(L, gpGame->day);
  return 1;
}

static int l_getWeek(lua_State *L) {
  lua_pushinteger(L, gpGame->week);
  return 1;
}

static int l_getMonth(lua_State *L) {
  lua_pushinteger(L, gpGame->month);
  return 1;
}

static void register_date_funcs(lua_State *L) {
  lua_register(L, "GetDay", l_getDay);
  lua_register(L, "GetWeek", l_getWeek);
  lua_register(L, "GetMonth", l_getMonth);
}

/************************************************ Player ********************************************************/

static int l_getNumPlayers(lua_State *L) {
	lua_pushinteger(L, gpGame->numPlayers);
	return 1;
}

static int l_getPlayer(lua_State *L) {
  int n = (int)luaL_checknumber(L, 1);
  deepbound_push(L, deepbind<playerData*>(&gpGame->players[n]));
  return 1;
}

static int l_getPlayerPersonality(lua_State *L) {
				playerData* p = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
				lua_pushinteger(L, p->personality);
				return 1;
}

static int l_getCurrentPlayer(lua_State *L) {
  deepbound_push(L, deepbind<playerData*>(gpCurPlayer));
  return 1;
}

static int l_getPlayerNumber(lua_State *L) {
				playerData* p = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
				lua_pushinteger(L, GetPlayerNumber(p));
				return 1;
}

static int l_getPlayerColor(lua_State *L) {
  playerData* p = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, p->color);
  return 1;
}

static int l_getNumHeroes(lua_State *L) {
  playerData* p = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, p->numHeroes);
  return 1;
}

static int l_getHero(lua_State *L) {
  playerData* p = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int n = (int)luaL_checknumber(L, 2);
  deepbound_push(L, deepbind<hero*>(&gpGame->heroes[p->heroesOwned[n]]));
  return 1;
}

static int l_getHeroForHire(lua_State *L) {
  playerData* p = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int n = (int)luaL_checknumber(L, 2);
  deepbound_push(L, deepbind<hero*>(&gpGame->heroes[p->heroesForPurchase[n]]));
  return 1;
}

static int l_giveResource(lua_State *L) {
  playerData *player = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
  int res = (int)luaL_checknumber(L, 2);
  int val = (int)luaL_checknumber(L, 3);
  player->resources[res] += val;
  return 0;
}

static int l_setResource(lua_State *L) {
  playerData *player = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
  int res = (int)luaL_checknumber(L, 2);
  int val = (int)luaL_checknumber(L, 3);
  player->resources[res] = val;
  return 0;
}

static int l_getResource(lua_State *L) {
  playerData *player = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int res = (int)luaL_checknumber(L, 2);
  lua_pushinteger(L, player->resources[res]);
  return 1;
}

static int l_shareVision(lua_State *L) {
				playerData* sourcePlayer = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
				playerData* destPlayer = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(2, 2));
				gpGame->ShareVision(GetPlayerNumber(sourcePlayer), GetPlayerNumber(destPlayer));
    return 0;
}

static int l_cancelShareVision(lua_State *L) {
				playerData* sourcePlayer = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
				playerData* destPlayer = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(2, 2));
				gpGame->CancelShareVision(GetPlayerNumber(sourcePlayer), GetPlayerNumber(destPlayer));
    return 0;
}

static int l_setDaysAfterTownLost(lua_State *L) {
  playerData *player = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int days = (int)luaL_checknumber(L, 2);
  player->daysLeftWithoutCastle = days;
  return 0;
}

static int l_getDaysAfterTownLost(lua_State *L) {
  playerData *player = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, player->daysLeftWithoutCastle);
  return 1;
}

static int l_revealMap(lua_State *L) {
  playerData *player = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 4));
  int x = (int)luaL_checknumber(L, 2);
  int y = (int)luaL_checknumber(L, 3);
  int radius = (int)luaL_checknumber(L, 4);

  for (int i = 0; i < gpGame->numPlayers; i++) {
    if (&gpGame->players[i] == player) {
      gpGame->SetVisibility(x, y, i, radius);
      break;
    }
  }
  return 1;
}

int l_GetBarrierTentVisited(lua_State* L) {
				playerData* plyd = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
				int tentcolor = luaL_checknumber(L, 2);
				if (plyd->barrierTentsVisited & (1 << tentcolor))
								lua_pushboolean(L, 1);
				else
								lua_pushboolean(L, 0);
				return 1;
}

int l_SetBarrierTentVisited(lua_State *L) {
	playerData* plyd = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
	int tentcolor = luaL_checknumber(L, 2);
	bool yes = CheckBoolean(L, 3);
	if (yes)
					plyd->SetBarrierTentVisited(tentcolor);
	else
					plyd->barrierTentsVisited &= ~(1 << tentcolor);
	return 0;
}

static void register_player_funcs(lua_State *L) {
  lua_register(L, "GetNumPlayers", l_getNumPlayers);
  lua_register(L, "GetPlayer", l_getPlayer);
		lua_register(L, "GetPlayerPersonality", l_getPlayerPersonality);
  lua_register(L, "GetCurrentPlayer", l_getCurrentPlayer);
		lua_register(L, "GetPlayerNumber", l_getPlayerNumber);
  lua_register(L, "GetPlayerColor", l_getPlayerColor);
  lua_register(L, "GetNumHeroes", l_getNumHeroes);
  lua_register(L, "GetHero", l_getHero);
  lua_register(L, "GetHeroForHire", l_getHeroForHire);
  lua_register(L, "GiveResource", l_giveResource);
  lua_register(L, "SetResource", l_setResource);
  lua_register(L, "GetResource", l_getResource);
  lua_register(L, "ShareVision", l_shareVision);
  lua_register(L, "CancelShareVision", l_cancelShareVision);
  lua_register(L, "SetDaysAfterTownLost", l_setDaysAfterTownLost);
  lua_register(L, "GetDaysAfterTownLost", l_getDaysAfterTownLost);
  lua_register(L, "RevealMap", l_revealMap);
		lua_register(L, "GetBarrierTentVisited", l_GetBarrierTentVisited);
  lua_register(L, "SetBarrierTentVisited", l_SetBarrierTentVisited);
}

/************************************************ Heroes ********************************************************/

static int l_getCurrentHero(lua_State *L) {
  deepbound_push(L, deepbind<hero*>(GetCurrentHero()));
  return 1;
}

static int l_grantSpell(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int sp = (int)luaL_checknumber(L, 2);
  hro->AddSpell(sp);
  return 0;
}

//this will only return true if the hero actually KNOWS the spell; sources like Spell Scrolls are ignored
static int l_hasSpell(lua_State *L) {
				hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
				int spell = (int)luaL_checknumber(L, 2);
				if (hro->spellsLearned[spell] == 1)
								lua_pushboolean(L, true);
				else
								lua_pushboolean(L, false);
				return 1;
}

static int l_hasSpellScroll(lua_State* L) {
				hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
				int spell = (int)luaL_checknumber(L, 2);
				for (int i = 0; i < MAX_ARTIFACTS; i++)
								if (hro->artifacts[i] == ARTIFACT_SPELL_SCROLL && hro->scrollSpell[i] == spell)
								{
												lua_pushboolean(L, true);
												return 1;
								}
				lua_pushboolean(L, false);
				return 1;
}

static int l_getHeroSex(lua_State* L) {
				hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
				if (HeroExtras[hro->idx]->GetHeroSex() == Sex::Male)
								lua_pushinteger(L, 0);
				else
								lua_pushinteger(L, 1);
				return 1;
}

static int l_setHeroSex(lua_State* L) {
				hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
				int sex = luaL_checkinteger(L, 2);
				if (sex == 0)
								HeroExtras[hro->idx]->SetHeroSex(Sex::Male);
				else
								HeroExtras[hro->idx]->SetHeroSex(Sex::Female);
				return 0;
}

static int l_forgetSpell(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int spell = (int)luaL_checknumber(L, 2);
  hro->spellsLearned[spell] = 0;
  return 0;
}

static int l_hasTroop(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
  int creature = (int)luaL_checknumber(L, 2);
  int quantity = (int)luaL_checknumber(L, 3);
  for (int i = 0; i < CREATURES_IN_ARMY; i++) {
    if (hro->army.creatureTypes[i] == creature && hro->army.quantities[i] >= quantity) {
      lua_pushboolean(L, true);
      return 1;
    }
  }
  lua_pushboolean(L, false);
  return 1;
}

static int l_getCreatureAmount(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int creature = (int)luaL_checknumber(L, 2);
  int quantity = 0;

  for (int i = 0; i < CREATURES_IN_ARMY; i++) {
    if (hro->army.creatureTypes[i] == creature) {
      quantity += hro->army.quantities[i];
    }
  }

  lua_pushinteger(L, quantity);
  return 1;
}

static int l_takeTroop(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
  int creature = (int)luaL_checknumber(L, 2);
  int quantity = (int)luaL_checknumber(L, 3);

  for (int i = 0; i < CREATURES_IN_ARMY; i++) {
    if (hro->army.creatureTypes[i] == creature) {
      if (hro->army.quantities[i] > quantity) {
        hro->army.quantities[i] -= quantity;
        break;
      }
      else if (hro->army.quantities[i] <= quantity) {
        quantity -= hro->army.quantities[i];
        hro->army.creatureTypes[i] = -1;
        hro->army.quantities[i] = 0;
      }
    }
  }
  return 0;
}

static int l_teleportHero(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
  int x = (int)luaL_checknumber(L, 2);
  int y = (int)luaL_checknumber(L, 3);

  // Working around a bug where TeleportTo will erroneously call
  // gpGame->SetVisibility(hero, viewX, viewY, heroViewingRadius)
  gpAdvManager->viewX = x;
  gpAdvManager->viewY = y;

  gpAdvManager->TeleportTo(hro, x, y, 0, 0);
  return 0;
}

static int l_getHeroName(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushstring(L, hro->name);
  return 1;
}

static int l_setHeroName(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  lua_pushstring(L, hro->name);
  strncpy_s(hro->name, luaL_checkstring(L, 2), _TRUNCATE);
  return 0;
}

static int l_getHeroInPool(lua_State *L) {
  int n = (int)luaL_checknumber(L, 1);
  deepbound_push(L, deepbind<hero*>(&gpGame->heroes[n]));
  return 1;
}

static int l_getHeroOwner(lua_State *L) {
  hero *hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  
  if (hro->ownerIdx < 0) {
    lua_pushnil(L);
  } else {
    deepbound_push(L, deepbind<playerData*>(&gpGame->players[hro->ownerIdx]));
  }

  return 1;
}

static int l_getHeroNumber(lua_State *L)
{
    hero *hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
    lua_pushinteger(L, hro->idx);
    return 1;
}

static int l_grantArtifact(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int art = (int)luaL_checknumber(L, 2);
  GiveArtifact(hro, art, 1, -1);
  return 0;
}

static int l_hasArtifact(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int art = (int)luaL_checknumber(L, 2);
  lua_pushboolean(L, hro->HasArtifact(art));
  return 1;
}

static int l_getArtifactAtIndex(lua_State* L) {
				hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
				int idx = (int)luaL_checknumber(L, 2);
				lua_pushinteger(L, hro->artifacts[idx]);
				return 1;
}

static int l_setArtifactAtIndex(lua_State* L) {
				hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
				int idx = (int)luaL_checknumber(L, 2);
				int artifact = (int)luaL_checknumber(L, 3);
				GiveTakeArtifactStat(hro, hro->artifacts[idx], 1);
				GiveTakeArtifactStat(hro, artifact, 0);
				hro->artifacts[idx] = artifact;
				return 0;
}

static int l_takeArtifact(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int art = (int)luaL_checknumber(L, 2);
  hro->TakeArtifact(art);
  return 0;
}

static int l_countEmptyArtifactSlots(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, hro->CountEmptyArtifactSlots());
  return 1;
}

static int l_countEmptyCreatureSlots(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, hro->CountEmptyCreatureSlots());
  return 1;
}

static int l_setExperiencePoints(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int points = (int)luaL_checknumber(L, 2);
  hro->experience = points;
  hro->CheckLevel();
  return 0;
}

static int l_getExperiencePoints(lua_State *L) {
  hero *hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, hro->experience);
  return 1;
}

static int l_setPrimarySkill(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
  int skill = (int)luaL_checknumber(L, 2);
  int amt = (int)luaL_checknumber(L, 3);
  hro->SetPrimarySkill(skill, amt);
  return 0;
}

static int l_getPrimarySkill(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int skill = (int)luaL_checknumber(L, 2);
  lua_pushinteger(L, hro->primarySkills[skill]);
  return 1;
}

static int l_setSpellpoints(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int points = (int)luaL_checknumber(L, 2);
  hro->spellpoints = points;
  return 0;
}

static int l_getSpellpoints(lua_State *L) {
  hero *hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, hro->spellpoints);
  return 1;
}

static int l_setSecondarySkill(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
  int skill = (int)luaL_checknumber(L, 2);
  int level = (int)luaL_checknumber(L, 3);
  hro->SetSS(skill, level);
  return 0;
}

static int l_getSecondarySkill(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int skill = (int)luaL_checknumber(L, 2);
  lua_pushinteger(L, hro->GetSSLevel(skill));
  return 1;
}

static int l_grantArmy(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
  int cr = (int)luaL_checknumber(L, 2);
  int n = (int)luaL_checknumber(L, 3);
  hro->army.Add(cr, n, -1);
  return 0;
}

static int l_getHeroMobility(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, hro->mobility);
  return 1;
}

static int l_setHeroMobility(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int mobility = (int)luaL_checknumber(L, 2);
  hro->mobility = mobility;
  return 0;
}

static int l_getHeroRemainingMobility(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, hro->remainingMobility);
  return 1;
}

static int l_setHeroRemainingMobility(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int remainingMobility = (int)luaL_checknumber(L, 2);
  hro->remainingMobility = remainingMobility;
  return 0;
}

static int l_getHeroX(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, hro->x);
  return 1;
}

static int l_getHeroY(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, hro->y);
  return 1;
}

static int l_getHeroLevel(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, hro->GetLevel());
  return 1;
}

static int l_getHeroTempMoraleBonuses(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, hro->tempMoraleBonuses);
  return 1;
}

static int l_setHeroTempMoraleBonuses(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int moraleBonus = (int)luaL_checknumber(L, 2);
  hro->tempMoraleBonuses = moraleBonus;
  return 0;
}

static int l_getHeroTempLuckBonuses(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, hro->tempLuckBonuses);
  return 1;
}

static int l_setHeroTempLuckBonuses(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int LuckBonus = (int)luaL_checknumber(L, 2);
  hro->tempLuckBonuses = LuckBonus;
  return 0;
}

static int l_grantSpellScroll(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int sp = (int)luaL_checknumber(L, 2);
  GiveArtifact(hro, ARTIFACT_SPELL_SCROLL, 1, sp);
  return 0;
}

static int l_getHeroFaction(lua_State *L) {
  hero *hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, hro->factionID);
  return 1;
}

static int l_setHeroFaction(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int newFaction = (int)luaL_checknumber(L, 2);
  hro->factionID = newFaction;
  return 0;
}

static int l_getHeroPortrait(lua_State *L)
{
    hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
    lua_pushinteger(L, hro->heroID);
    return 1;
}

static int l_setHeroPortrait(lua_State *L)
{
    hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
    int id = (int)luaL_checknumber(L, 2);
    hro->heroID = id;
    return 0;
}

static void register_hero_funcs(lua_State *L) {
  lua_register(L, "GetCurrentHero", l_getCurrentHero);
  lua_register(L, "GrantSpell", l_grantSpell);
		lua_register(L, "HasSpell", l_hasSpell);
  lua_register(L, "ForgetSpell", l_forgetSpell);
  lua_register(L, "HasTroop", l_hasTroop);
  lua_register(L, "GetCreatureAmount", l_getCreatureAmount);
  lua_register(L, "TakeTroop", l_takeTroop);
  lua_register(L, "TeleportHero", l_teleportHero);
  lua_register(L, "GetHeroName", l_getHeroName);
  lua_register(L, "SetHeroName", l_setHeroName);
  lua_register(L, "GetHeroInPool", l_getHeroInPool);
  lua_register(L, "GetHeroOwner", l_getHeroOwner);
  lua_register(L, "GetHeroNumber", l_getHeroNumber);
  lua_register(L, "GrantArtifact", l_grantArtifact);
  lua_register(L, "HasArtifact", l_hasArtifact);
		lua_register(L, "GetArtifactAtIndex", l_getArtifactAtIndex);
		lua_register(L, "SetArtifactAtIndex", l_setArtifactAtIndex);
  lua_register(L, "TakeArtifact", l_takeArtifact);
  lua_register(L, "CountEmptyArtifactSlots", l_countEmptyArtifactSlots);
  lua_register(L, "CountEmptyCreatureSlots", l_countEmptyCreatureSlots);
  lua_register(L, "SetExperiencePoints", l_setExperiencePoints);
  lua_register(L, "GetExperiencePoints", l_getExperiencePoints);
  lua_register(L, "SetPrimarySkill", l_setPrimarySkill);
  lua_register(L, "GetPrimarySkill", l_getPrimarySkill);
  lua_register(L, "SetSpellpoints", l_setSpellpoints);
  lua_register(L, "GetSpellpoints", l_getSpellpoints);
  lua_register(L, "SetSecondarySkill", l_setSecondarySkill);
  lua_register(L, "GetSecondarySkill", l_getSecondarySkill);
  lua_register(L, "GrantArmy", l_grantArmy);
  lua_register(L, "GetHeroMobility", l_getHeroMobility);
  lua_register(L, "SetHeroMobility", l_setHeroMobility);
  lua_register(L, "GetHeroRemainingMobility", l_getHeroRemainingMobility);
  lua_register(L, "SetHeroRemainingMobility", l_setHeroRemainingMobility);
  lua_register(L, "GetHeroX", l_getHeroX);
  lua_register(L, "GetHeroY", l_getHeroY);
  lua_register(L, "GetHeroLevel", l_getHeroLevel);
  lua_register(L, "GetHeroTempMoraleBonuses", l_getHeroTempMoraleBonuses);
  lua_register(L, "SetHeroTempMoraleBonuses", l_setHeroTempMoraleBonuses);
  lua_register(L, "GetHeroTempLuckBonuses", l_getHeroTempLuckBonuses);
  lua_register(L, "SetHeroTempLuckBonuses", l_setHeroTempLuckBonuses);
  lua_register(L, "GrantSpellScroll", l_grantSpellScroll);
		lua_register(L, "HasSpellScroll", l_hasSpellScroll);
		lua_register(L, "GetHeroSex", l_getHeroSex);
		lua_register(L, "SetHeroSex", l_setHeroSex);
  lua_register(L, "GetHeroFaction", l_getHeroFaction);
  lua_register(L, "SetHeroFaction", l_setHeroFaction);
  lua_register(L, "GetHeroPortrait", l_getHeroPortrait);
  lua_register(L, "SetHeroPortrait", l_setHeroPortrait);
}

/************************************** Map *******************************************/

// FIXME: What is this 3?
static int l_mapSetObject(lua_State *L) {
  int x = (int)luaL_checknumber(L, 1);
  int y = (int)luaL_checknumber(L, 2);
  int obj = (int)luaL_checknumber(L, 3);

  mapCell* cell = gpAdvManager->GetCell(x, y);
  cell->overlayIndex = 3;
  cell->objectIndex = obj;
  cell->objType = obj & 0x7F;
  return 0;
}

/*static int l_mapEmptyCheck(lua_State *L) {
	int x = (int)luaL_checknumber(L, 1);
	int y = (int)luaL_checknumber(L, 2);

	mapCell* cell = gpAdvManager->GetCell(x, y);
	if (cell->objType & TILE_HAS_EVENT) {
		lua_pushboolean(L, false);
		return 1;
	}
	lua_pushboolean(L, true);
	return 1;
}

static int l_mapPutResource(lua_State *L) {
	int x = (int)luaL_checknumber(L, 1);
	int y = (int)luaL_checknumber(L, 2);
	int resIdx = (int)luaL_checknumber(L, 3);
	int resQty = (int)luaL_checknumber(L, 4);

	lua_pushinteger(L, gpAdvManager->MapPutResource(x, y, resIdx, resQty));
	return 1;
}*/

static int l_getShrineSpell(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int spell = GetShrineSpell(x, y);
				if (spell != -1)
				{
								lua_pushinteger(L, spell);
								return 1;
				}
				return 0;
}

static int l_setShrineSpell(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int spell = (int)luaL_checknumber(L, 3);
				SetShrineSpell(x, y, spell);
				return 0;
}

static int l_getWitchHutSkill(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int skill = GetWitchHutSkill(x, y);
				if (skill != -1)
				{
								lua_pushinteger(L, skill);
								return 1;
				}
				return 0;
}

static int l_setWitchHutSkill(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int skill = (int)luaL_checknumber(L, 3);
				SetWitchHutSkill(x, y, skill);
				return 0;
}

static int l_getSignText(lua_State *L)
{
				const char* signTexts[4] = {
								"See Rock City",
								"This space for rent",
								"Next sign 50 miles",
								"Burma shave"
				}; //this DOES exist in the original code as off_4F70C8 but I don't know how to use it
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				mapCell* loc = gpAdvManager->GetCell(x, y);
				SignExtra* sign = (SignExtra *)ppMapExtra[loc->extraInfo];
				if (strlen(&sign->message) < 1)
								lua_pushstring(L, signTexts[x % 4]);
				else
								lua_pushstring(L, &sign->message);
				return 1;
}

static int l_setSignText(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				const char* text = (char*)luaL_checkstring(L, 3);
				mapCell* loc = gpAdvManager->GetCell(x, y);
    SignExtra* sign = (SignExtra*)ALLOC(sizeof *sign + strlen(text));
    SignExtra* signOld = (SignExtra *)ppMapExtra[loc->extraInfo];
    memcpy(sign, signOld, sizeof *sign);
    FREE(signOld);
				strcpy(&sign->message, text);
    ppMapExtra[loc->extraInfo] = sign;
				return 0;
}

static int l_setPlayerVisitedShrine(lua_State *L)
{
				playerData* p = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 4));
				int x = (int)luaL_checknumber(L, 2);
				int y = (int)luaL_checknumber(L, 3);
				bool yes = CheckBoolean(L, 4);
				if (yes)
								PlayerVisitedObject[x][y] |= 1u << p->color;
				else
								PlayerVisitedObject[x][y] &= ~(1u << p->color);
				return 0;
}

static int l_getPlayerVisitedShrine(lua_State *L)
{
				playerData* p = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
				int x = (int)luaL_checknumber(L, 2);
				int y = (int)luaL_checknumber(L, 3);
				int answer = (PlayerVisitedObject[x][y] >> p->color) & 1u;
				if (answer)
								lua_pushboolean(L, true);
				else
								lua_pushboolean(L, false);
				return 1;
}

static int l_getMineId(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int id = gpGame->GetMineId(x, y);
				if (id == -1)
								lua_pushnil(L);
				else
								lua_pushinteger(L, id);
				return 1;
}

static int l_getMineOwner(lua_State *L)
{
				int id = (int)luaL_checknumber(L, 1);
				mine* mn = &gpGame->mines[id];
				if (mn->owner == -1)
								lua_pushnil(L);
				else
								deepbound_push(L, deepbind<playerData*>(&gpGame->players[mn->owner]));
				return 1;
}

static int l_setMineOwner(lua_State *L)
{
				int id = (int)luaL_checknumber(L, 1);
				int idx;
				if (lua_isnil(L, 2))
				{
								lua_pop(L, 2);
								idx = -1;
				}
				else
								idx = GetPlayerNumber((playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(2, 2)));
				gpGame->ClaimMine(id, idx);
				return 0;
}

static int l_getMineGuards(lua_State *L)
{
				int id = (int)luaL_checknumber(L, 1);
				mine* mn = &gpGame->mines[id];
				lua_pushinteger(L, mn->guardianType);
				return 1;
}

static int l_getMineGuardCount(lua_State *L)
{
				int id = (int)luaL_checknumber(L, 1);
				mine* mn = &gpGame->mines[id];
				lua_pushinteger(L, mn->guardianQty);
				return 1;
}

static int l_setMineGuards(lua_State *L)
{
				int id = (int)luaL_checknumber(L, 1);
				int type = (int)luaL_checknumber(L, 2);
				int qty = (int)luaL_checknumber(L, 3);
				mine* mn = &gpGame->mines[id];
				mn->guardianType = type;
				mn->guardianQty = qty;
				return 0;
}

static int l_getDwellingQuantity(lua_State* L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				int locType = cell->objType & 0x7F;
				if (locType == LOCATION_EXPANSION_DWELLING)
								lua_pushinteger(L, cell->extraInfo / 8);
				else if (locType == LOCATION_TROLL_BRIDGE || locType == LOCATION_CITY_OF_DEAD || locType == LOCATION_DRAGON_CITY)
				{
								if (cell->extraInfo & 0x100)
												lua_pushinteger(L, cell->extraInfo & (~0x100));
								else
												lua_pushinteger(L, cell->extraInfo);
				}
				else
								lua_pushinteger(L, cell->extraInfo);
				return 1;
}

static int l_setDwellingQuantity(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int qty = (int)luaL_checknumber(L, 3);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				int locType = cell->objType & 0x7F;
				if (locType == LOCATION_TROLL_BRIDGE || locType == LOCATION_CITY_OF_DEAD || locType == LOCATION_DRAGON_CITY)
				{
								if (cell->extraInfo & 0x100)
												cell->extraInfo = qty | 0x100;
								else
												cell->extraInfo = qty;
				}
				else if (locType == LOCATION_EXPANSION_DWELLING)
								cell->extraInfo = qty * 8 + cell->extraInfo % 8;
				else
								cell->extraInfo = qty;
				return 0;
}

static int l_dwellingHasGuards(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				int locType = cell->objType & 0x7F;
				if (locType == LOCATION_TROLL_BRIDGE || locType == LOCATION_CITY_OF_DEAD || locType == LOCATION_DRAGON_CITY)
								if (cell->extraInfo & 0x100)
												lua_pushboolean(L, true);
								else
												lua_pushboolean(L, false);
				else
								lua_pushboolean(L, false);
				return 1;
}

static int l_setDwellingHasGuards(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				bool yes = CheckBoolean(L, 3);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				int locType = cell->objType & 0x7F;
				if (locType == LOCATION_TROLL_BRIDGE || locType == LOCATION_CITY_OF_DEAD || locType == LOCATION_DRAGON_CITY)
								if (yes)
												cell->extraInfo |= 0x100;
								else if (!yes)
												cell->extraInfo &= ~0x100;
				return 0;
}

static int l_getCampfireResource(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_CAMPFIRE)
				{
								lua_pushnil(L);
								return 1;
				}
				lua_pushinteger(L, cell->extraInfo & 0xF);
				return 1;
}

static int l_getCampfireResourceCount(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_CAMPFIRE && (cell->objType & 0x7F) != LOCATION_LEAN_TO)
				{
								lua_pushnil(L);
								return 1;
				}
				lua_pushinteger(L, cell->extraInfo >> 4);
				return 1;
}

static int l_setCampfireResource(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int res = (int)luaL_checknumber(L, 3);
				int qty = (int)luaL_checknumber(L, 4);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_CAMPFIRE)
								return 0;
				cell->extraInfo = (qty << 4) + res;
				return 0;
}

static int l_getLeanToResource(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_LEAN_TO)
				{
								lua_pushnil(L);
								return 1;
				}
				if (cell->extraInfo == 0)
								lua_pushinteger(L, -1);
				else
								lua_pushinteger(L, (cell->extraInfo & 0xF) - 1);
				return 1;
}

static int l_setLeanToResource(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int res = (int)luaL_checknumber(L, 3);
				int qty = (int)luaL_checknumber(L, 4);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_LEAN_TO)
								return 0;
				cell->extraInfo = (qty << 4) + res + 1;
				return 0;
}

static int l_getWagonType(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int type = GetWagonType(x, y);
				if (type == -1)
								lua_pushnil(L);
				else
								lua_pushinteger(L, type);
				return 1;
}

static int l_getWagonResource(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int type = GetWagonType(x, y);
				if (type != 1)
								lua_pushnil(L);
				else
								lua_pushinteger(L, (gpAdvManager->GetCell(x, y)->extraInfo & 0xF) - 1);
				return 1;
}

static int l_getWagonResourceCount(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int type = GetWagonType(x, y);
				if (type != 1)
								lua_pushnil(L);
				else
								lua_pushinteger(L, gpAdvManager->GetCell(x, y)->extraInfo >> 4);
				return 1;
}

static int l_getWagonArtifact(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int type = GetWagonType(x, y);
				if (type != 2)
								lua_pushnil(L);
				else
								lua_pushinteger(L, gpAdvManager->GetCell(x, y)->extraInfo & 0x7F);
				return 1;
}

static int l_setWagonResource(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int res = (int)luaL_checknumber(L, 3);
				int count = (int)luaL_checknumber(L, 4);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if (count > 7)
								count = 7;
				if ((cell->objType & 0x7F) != LOCATION_WAGON)
								return 0;
				if (res < 0 || count <= 0)
								cell->extraInfo = 0;
				else
								cell->extraInfo = (count << 4) + res + 1;
				return 0;
}

static int l_setWagonArtifact(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int artifact = (int)luaL_checknumber(L, 3);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_WAGON)
								return 0;
				if (artifact < 0)
								cell->extraInfo = 0;
				else
								cell->extraInfo = (artifact & 0x7F) | 0x80;
				return 0;
}

static int l_getSkeletonArtifact(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_SKELETON)
				{
								lua_pushnil(L);
								return 1;
				}
				int artifact = cell->extraInfo;
				artifact -= 2;
				//the value cannot be pushed directly; cell->extraInfo is unsigned
				//when it is 1 (empty Skeleton), the value pushed would not be -1, but rather 4294967295
				lua_pushinteger(L, artifact);
				return 1;
}

static int l_setSkeletonArtifact(lua_State *L) {
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int artifact = (int)luaL_checknumber(L, 3);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_SKELETON)
								return 0;
				cell->extraInfo = artifact + 2;
				return 0;
}

static int l_getResourcePileCount(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_RESOURCE)
				{
								lua_pushnil(L);
								return 1;
				}
				lua_pushinteger(L, cell->extraInfo);
				return 1;
}

static int l_setResourcePileCount(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int qty = (int)luaL_checknumber(L, 3);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_RESOURCE)
								return 0;
				cell->extraInfo = qty;
				return 0;
}

static int l_getTreasureChestType(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_TREASURE_CHEST)
				{
								lua_pushnil(L);
								return 1;
				}
				if (cell->extraInfo & 0x100)
								lua_pushinteger(L, 2);
				else
								lua_pushinteger(L, 1);
				return 1;
}

static int l_getTreasureChestLevel(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_TREASURE_CHEST)
				{
								lua_pushnil(L);
								return 1;
				}
				if (cell->extraInfo & 0x100)
								lua_pushnil(L);
				else
								lua_pushinteger(L, cell->extraInfo);
				return 1;
}

static int l_getTreasureChestArtifact(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_TREASURE_CHEST)
				{
								lua_pushnil(L);
								return 1;
				}
				if (cell->extraInfo & 0x100)
								lua_pushinteger(L, cell->extraInfo ^ 0x100);
				else
								lua_pushnil(L);
				return 1;
}

static int l_setTreasureChestLevel(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int lvl = (int)luaL_checknumber(L, 3);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_TREASURE_CHEST)
								return 0;
				cell->extraInfo = lvl;
				return 0;
}

static int l_setTreasureChestArtifact(lua_State *L)
{
				int x = (int)luaL_checknumber(L, 1);
				int y = (int)luaL_checknumber(L, 2);
				int artifact = (int)luaL_checknumber(L, 3);
				mapCell* cell = gpAdvManager->GetCell(x, y);
				if ((cell->objType & 0x7F) != LOCATION_TREASURE_CHEST)
								return 0;
				cell->extraInfo = artifact | 0x100;
				return 0;
}

static int l_getMagicGardenResource(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_MAGIC_GARDEN)
        lua_pushinteger(L, cell->extraInfo - 1);
    else
        lua_pushnil(L);
    return 1;
}

static int l_setMagicGardenResource(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int resource = (int)luaL_checknumber(L, 3);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_MAGIC_GARDEN)
        cell->extraInfo = resource + 1;
    return 0;
}

static int l_getWaterWheelGold(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_WATERWHEEL)
        lua_pushinteger(L, cell->extraInfo);
    else
        lua_pushnil(L);
    return 1;
}

static int l_setWaterWheelGold(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int qty = (int)luaL_checknumber(L, 3);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_WATERWHEEL)
        cell->extraInfo = qty;
    return 0;
}

static int l_getWindmillResource(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_WINDMILL)
        if (cell->extraInfo != 99)
            lua_pushinteger(L, cell->extraInfo);
        else
            lua_pushinteger(L, -1);
    else
        lua_pushnil(L);
    return 1;
}

static int l_setWindmillResource(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int resource = (int)luaL_checknumber(L, 3);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_WINDMILL)
        if (resource != -1)
            cell->extraInfo = resource;
        else
            cell->extraInfo = 99;
    return 0;
}

static int l_getSeaChestTreasure(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_SEA_CHEST)
    {
        if (cell->extraInfo & 0x100)
            lua_pushinteger(L, cell->extraInfo);
        else if (cell->extraInfo != 0)
            lua_pushinteger(L, -2);
        else
            lua_pushinteger(L, -1);
    }
    else
        lua_pushnil(L);
    return 1;
}

static int l_setSeaChestTreasure(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int treasure = (int)luaL_checknumber(L, 3);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_SEA_CHEST)
    {
        if (treasure == -1)
            cell->extraInfo = 0;
        else if (treasure >= 0)
            cell->extraInfo = treasure | 0x100;
        else
            cell->extraInfo = 1;
    }
    return 0;
}

static int l_getFlotsamLevel(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_FLOTSAM)
        lua_pushinteger(L, cell->extraInfo);
    else
        lua_pushnil(L);
    return 1;
    //0: empty
    //1: 5 wood
    //2: 5 wood, 200 gold
    //3: 10 wood, 500 gold
}

static int l_setFlotsamLevel(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int level = (int)luaL_checknumber(L, 3);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_FLOTSAM)
            cell->extraInfo = level;
    return 0;
}

static int l_getShipwreckSurvivorArtifact(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_SHIPWRECK_SURVIVOR)
        lua_pushinteger(L, cell->extraInfo);
    else
        lua_pushnil(L);
    return 1;
}

static int l_setShipwreckSurvivorArtifact(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int artifact = (int)luaL_checknumber(L, 3);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_SHIPWRECK_SURVIVOR)
        cell->extraInfo = artifact;
    return 0;
}

static int l_getDerelictShipVisited(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_DERELICT_SHIP)
    {
        if (cell->extraInfo == 1)
            lua_pushboolean(L, true);
        else
            lua_pushboolean(L, false);
    }
    else
        lua_pushnil(L);
    return 1;
}

static int l_setDerelictShipVisited(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    bool yesno = CheckBoolean(L, 3);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_DERELICT_SHIP)
        if (yesno)
            cell->extraInfo = 1;
        else
            cell->extraInfo = 4;
    //naturally, the game initializes the object with 3, 4 or 5, but they seem to make no difference
    return 0;
}

static int l_getShipwreckType(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_SHIPWRECK)
        lua_pushinteger(L, cell->extraInfo);
    else
        lua_pushnil(L);
    return 1;
}

static int l_setShipwreckType(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int type = (int)luaL_checknumber(L, 3);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_SHIPWRECK)
        cell->extraInfo = type;
    return 0;
    //1: empty
    //2: 1000 gold
    //3: 2000 gold
    //4: 5000 gold
    //anything else: 2000 gold + 1 random artifact
}

static int l_getGraveyardVisited(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_GRAVEYARD)
    {
        if (cell->extraInfo == 1)
            lua_pushboolean(L, true);
        else
            lua_pushboolean(L, false);
    }
    else
        lua_pushnil(L);
    return 1;
}

static int l_setGraveyardVisited(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    bool yesno = CheckBoolean(L, 3);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_GRAVEYARD)
        if (yesno)
            cell->extraInfo = 1;
        else
            cell->extraInfo = 4;
    //naturally, the game initializes the object with 2, 3 or 4, but they seem to make no difference
    return 0;
}

static int l_getDaemonCaveType(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_DAEMON_CAVE)
        lua_pushinteger(L, cell->extraInfo);
    else
        lua_pushnil(L);
    return 1;
}

static int l_setDaemonCaveType(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int type = (int)luaL_checknumber(L, 3);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_DAEMON_CAVE)
        cell->extraInfo = type;
    return 0;
    //1: empty
    //2: 1000 experience
    //3: 1000 experience + 1 random artifact (uses 4 if backpack is full)
    //4: 1000 experience + 2500 gold
    //5: pay 2500 gold or die
}

static int l_getJailHero(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_JAIL)
        deepbound_push(L, deepbind<hero*>(&gpGame->heroes[cell->extraInfo]));
    else
        lua_pushnil(L);
    return 1;
}

static int l_setJailHero(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(3, 3));
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) == LOCATION_JAIL)
        cell->extraInfo = hro->idx;
    return 0;
}

static int l_getSphinxData(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) != LOCATION_SPHINX)
        lua_pushnil(L);
    else
    {
        SphinxExtra* sphinx = (SphinxExtra*)ppMapExtra[cell->extraInfo];
        lua_newtable(L);
        lua_pushstring(L, "visited");
        lua_pushboolean(L, !sphinx->unclaimed);
        lua_settable(L, -3);
        lua_pushstring(L, "resourceReward");
        lua_newtable(L);
        for (int i = 0; i < NUM_RESOURCES; i++)
        {
            lua_pushinteger(L, i);
            lua_pushinteger(L, sphinx->resourceReward[i]);
            lua_settable(L, -3);
        }
        lua_settable(L, -3);
        lua_pushstring(L, "artifactReward");
        lua_pushinteger(L, sphinx->artifactReward);
        lua_settable(L, -3);
        lua_pushstring(L, "answers");
        lua_newtable(L);
        for (int i = 0; i < sphinx->numAnswers; i++)
        {
            lua_pushinteger(L, i + 1);
            lua_pushstring(L, sphinx->answers[i]);
            lua_settable(L, -3);
        }
        lua_settable(L, -3);
        lua_pushstring(L, "riddle");
        lua_pushstring(L, &sphinx->riddle);
        lua_settable(L, -3);
    }
    return 1;
}

static int l_setSphinxData(lua_State *L)
{
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    mapCell* cell = gpAdvManager->GetCell(x, y);
    if ((cell->objType & 0x7F) != LOCATION_SPHINX)
        return 0;
    std::string riddle;
    lua_pushstring(L, "riddle");
    lua_gettable(L, 3);
    riddle = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    SphinxExtra* sphinx = (SphinxExtra*)ALLOC(sizeof(SphinxExtra) + riddle.size());
    strcpy(&sphinx->riddle, riddle.c_str());
    lua_pushstring(L, "visited");
    lua_gettable(L, 3);
    sphinx->unclaimed = !CheckBoolean(L, -1);
    lua_pop(L, 1);
    lua_pushstring(L, "resourceReward");
    lua_gettable(L, 3);
    for (int i = 0; i < NUM_RESOURCES; i++)
    {
        lua_pushinteger(L, i);
        lua_gettable(L, -2);
        sphinx->resourceReward[i] = luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_pushstring(L, "artifactReward");
    lua_gettable(L, 3);
    sphinx->artifactReward = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    lua_pushstring(L, "answers");
    lua_gettable(L, 3);
    for (sphinx->numAnswers = 0; sphinx->numAnswers < 8; sphinx->numAnswers++)
    {
        lua_pushinteger(L, sphinx->numAnswers + 1);
        lua_gettable(L, -2);
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            break;
        }
        strncpy_s(sphinx->answers[sphinx->numAnswers], luaL_checkstring(L, -1), _TRUNCATE);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    FREE(ppMapExtra[cell->extraInfo]);
    ppMapExtra[cell->extraInfo] = sphinx;
    return 0;
}

static int l_mapPutArmy(lua_State *L) {
  int x = (int)luaL_checknumber(L, 1);
  int y = (int)luaL_checknumber(L, 2);
  int monIdx = (int)luaL_checknumber(L, 3);
  int monQty = (int)luaL_checknumber(L, 4);

  lua_pushinteger(L, gpAdvManager->MapPutArmy(x, y, monIdx, monQty));
  return 1;
}

/* Note: This doesn't really work, at least not in the way you want
 * EraseObj has special casing for a number of objects which are normally deleted in the game
 * (e.g.: monsters, resources, jails). This special casing will make it find other tiles (namely, shadows)
 * associated with the deleted object and delete them as well.
 * 
 * If you try to delete other objects, you will need to manually call this for every single tile -- and
 * if there are multiple overlays on a tile, you don't control which one is deleted. Furthermore,
 * if the tile was impassable, it will still be impassable. It appears that the impassability of tiles
 * is controlled by some mixture of display flag 0x20 (or was it 0x80? I forget) and other stuff
 * (like whether it has an object?). We still need to figure out exactly how that works.
 */
static int l_mapEraseObj(lua_State *L) {
  int x = (int)luaL_checknumber(L, 1);
  int y = (int)luaL_checknumber(L, 2);
  mapCell *cell = gpAdvManager->GetCell(x, y);
  gpAdvManager->EraseObj(cell, x, y);
  gpAdvManager->CompleteDraw(0);
  return 0;
}

static int l_mapFizzleObj(lua_State *L) {
	SAMPLE2 res;
	int x = (int)luaL_checknumber(L, 1);
	int y = (int)luaL_checknumber(L, 2);
	bool snd = CheckBoolean(L, 3);
	mapCell *cell = gpAdvManager->GetCell(x, y);
	gpAdvManager->CompleteDraw(0);
	gpWindowManager->SaveFizzleSource(gMapViewportRegion._left, gMapViewportRegion._top, gMapViewportRegion.getWidth(), gMapViewportRegion.getHeight());
	if (snd) {
		if (!PlaySoundEffect((!CheckLocationItem(cell)?"killfade":("pickup0"+std::to_string(Random(1, 7)))), SND_DONT_WAIT, &res)) {
			snd = false;
		}
	}
	gpAdvManager->EraseObj(cell, x, y);
	gpAdvManager->CompleteDraw(0);
	PollSound();
	gpWindowManager->FizzleForward(gMapViewportRegion._left, gMapViewportRegion._top, gMapViewportRegion.getWidth(), gMapViewportRegion.getHeight(), -1, 0, 0);
	if (snd) {
		WaitEndSample(res, res.sample);
	}
	return 0;
}

static int l_mapSetTerrainTile(lua_State *L) {
  int x = (int)luaL_checknumber(L, 1);
  int y = (int)luaL_checknumber(L, 2);
  int tileno = (int)luaL_checknumber(L, 3);

  __int8 flip = MAP_CELL_NO_FLIP;
  if (lua_gettop(L) >= 4) {
    flip = (__int8)luaL_checknumber(L, 4);
  }

  mapCell *cell = gpAdvManager->GetCell(x, y);
  cell->groundIndex = tileno;
  cell->SetTileFlip(flip);
  gpAdvManager->CompleteDraw(0);
  return 0;
}

static void register_map_funcs(lua_State *L) {
  lua_register(L, "MapSetObject", l_mapSetObject);
		//lua_register(L, "MapEmptyCheck", l_mapEmptyCheck);
		//lua_register(L, "MapPutResource", l_mapPutResource);
  lua_register(L, "MapPutArmy", l_mapPutArmy);
  lua_register(L, "MapEraseSquare", l_mapEraseObj);
  lua_register(L, "MapFizzle", l_mapFizzleObj);
  lua_register(L, "MapSetTileTerrain", l_mapSetTerrainTile);
		lua_register(L, "GetShrineSpell", l_getShrineSpell);
		lua_register(L, "SetShrineSpell", l_setShrineSpell);
		lua_register(L, "GetWitchHutSkill", l_getWitchHutSkill);
		lua_register(L, "SetWitchHutSkill", l_setWitchHutSkill);
		lua_register(L, "GetSignText", l_getSignText);
		lua_register(L, "SetSignText", l_setSignText);
		lua_register(L, "GetPlayerVisitedShrine", l_getPlayerVisitedShrine);
		lua_register(L, "SetPlayerVisitedShrine", l_setPlayerVisitedShrine);
		lua_register(L, "GetPlayerVisitedWitchHut", l_getPlayerVisitedShrine);
		lua_register(L, "SetPlayerVisitedWitchHut", l_setPlayerVisitedShrine);
		lua_register(L, "GetMineId", l_getMineId);
		lua_register(L, "GetMineOwner", l_getMineOwner);
		lua_register(L, "SetMineOwner", l_setMineOwner);
		lua_register(L, "GetMineGuards", l_getMineGuards);
		lua_register(L, "GetMineGuardCount", l_getMineGuardCount);
		lua_register(L, "SetMineGuards", l_setMineGuards);
		lua_register(L, "GetDwellingQuantity", l_getDwellingQuantity);
		lua_register(L, "SetDwellingQuantity", l_setDwellingQuantity);
		lua_register(L, "DwellingHasGuards", l_dwellingHasGuards);
		lua_register(L, "SetDwellingHasGuards", l_setDwellingHasGuards);
		lua_register(L, "GetCampfireResource", l_getCampfireResource);
		lua_register(L, "GetCampfireResourceCount", l_getCampfireResourceCount);
		lua_register(L, "SetCampfireResource", l_setCampfireResource);
		lua_register(L, "GetLeanToResource", l_getLeanToResource);
		lua_register(L, "GetLeanToResourceCount", l_getCampfireResourceCount);
		lua_register(L, "SetLeanToResource", l_setLeanToResource);
		lua_register(L, "GetWagonType", l_getWagonType);
		lua_register(L, "GetWagonResource", l_getWagonResource);
		lua_register(L, "GetWagonResourceCount", l_getWagonResourceCount);
		lua_register(L, "GetWagonArtifact", l_getWagonArtifact);
		lua_register(L, "SetWagonResource", l_setWagonResource);
		lua_register(L, "SetWagonArtifact", l_setWagonArtifact);
		lua_register(L, "GetSkeletonArtifact", l_getSkeletonArtifact);
		lua_register(L, "SetSkeletonArtifact", l_setSkeletonArtifact);
		lua_register(L, "GetResourcePileCount", l_getResourcePileCount);
		lua_register(L, "SetResourcePileCount", l_setResourcePileCount);
		lua_register(L, "GetTreasureChestType", l_getTreasureChestType);
		lua_register(L, "GetTreasureChestLevel", l_getTreasureChestLevel);
		lua_register(L, "GetTreasureChestArtifact", l_getTreasureChestArtifact);
		lua_register(L, "SetTreasureChestLevel", l_setTreasureChestLevel);
		lua_register(L, "SetTreasureChestArtifact", l_setTreasureChestArtifact);
  lua_register(L, "GetWindmillResource", l_getWindmillResource);
  lua_register(L, "SetWindmillResource", l_setWindmillResource);
  lua_register(L, "GetWaterWheelGold", l_getWaterWheelGold);
  lua_register(L, "SetWaterWheelGold", l_setWaterWheelGold);
  lua_register(L, "GetMagicGardenResource", l_getMagicGardenResource);
  lua_register(L, "SetMagicGardenResource", l_setMagicGardenResource);
  lua_register(L, "GetSeaChestTreasure", l_getSeaChestTreasure);
  lua_register(L, "SetSeaChestTreasure", l_setSeaChestTreasure);
  lua_register(L, "GetFlotsamLevel", l_getFlotsamLevel);
  lua_register(L, "SetFlotsamLevel", l_setFlotsamLevel);
  lua_register(L, "GetShipwreckSurvivorArtifact", l_getShipwreckSurvivorArtifact);
  lua_register(L, "SetShipwreckSurvivorArtifact", l_setShipwreckSurvivorArtifact);
  lua_register(L, "GetDerelictShipVisited", l_getDerelictShipVisited);
  lua_register(L, "SetDerelictShipVisited", l_setDerelictShipVisited);
  lua_register(L, "GetShipwreckType", l_getShipwreckType);
  lua_register(L, "SetShipwreckType", l_setShipwreckType);
  lua_register(L, "GetGraveyardVisited", l_getGraveyardVisited);
  lua_register(L, "SetGraveyardVisited", l_setGraveyardVisited);
  lua_register(L, "GetDaemonCaveType", l_getDaemonCaveType);
  lua_register(L, "SetDaemonCaveType", l_setDaemonCaveType);
  lua_register(L, "GetJailHero", l_getJailHero);
  lua_register(L, "SetJailHero", l_setJailHero);
  lua_register(L, "GetSphinxData", l_getSphinxData);
  lua_register(L, "SetSphinxData", l_setSphinxData);
}

/************************************** Town *******************************************/

static int l_getCurrentTown(lua_State *L) {
  deepbound_push(L, deepbind<town*>(gpTownManager->castle));
  return 1;
}

static int l_hasVisitingHero(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushboolean(L, twn->visitingHeroIdx >= 0);
  return 1;
}

static int l_getVisitingHero(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  deepbound_push(L, deepbind<hero*>(&gpGame->heroes[twn->visitingHeroIdx]));
  return 1;
}

static int l_buildInCurrentTown(lua_State *L) {
  int obj = (int)luaL_checknumber(L, 1);
  gpTownManager->BuildObj(obj);
  return 0;
}

static int l_getTown(lua_State *L) {
  int index = (int)luaL_checknumber(L, 1);
  if (index < MAX_TOWNS) {
    deepbound_push(L, deepbind<town*>(&gpGame->castles[index]));
  }
  return 1;
}

static int l_getTownName(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushstring(L, twn->name);
  return 1;
}

static int l_setTownName(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  strcpy(twn->name, luaL_checkstring(L, 2));
  return 1;
}

static int l_getTownByName(lua_State *L) {
  char *name = (char*)luaL_checkstring(L, 1);
  for (int i = 0; i < MAX_TOWNS; i++) {
    if (strcmp(gpGame->castles[i].name, name) == 0) {
      deepbound_push(L, deepbind<town*>(&gpGame->castles[i]));
      return 1;
    }
  }
  lua_pushinteger(L, -1);
  return 1;
}

static int l_getPlayerTown(lua_State *L) {
  playerData *player = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int index = (int)luaL_checknumber(L, 2);

  if (index < MAX_TOWNS) {
						deepbound_push(L, deepbind<town*>(&gpGame->castles[player->castlesOwned[index]]));
						return 1;
  }
  return 0;
}

static int l_buildInTown(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int building = (int)luaL_checknumber(L, 2);
		if (gpTownManager->castle == NULL)
						twn->BuildBuilding(building);
		else if (gpTownManager->castle->idx == twn->idx)
						gpTownManager->BuildObj(building);
		else
						twn->BuildBuilding(building);
  return 0;
}

static int l_getTownFaction(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, twn->factionID);
  return 1;
}

static int l_setTownFaction(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  FACTION faction = (FACTION)(int)luaL_checknumber(L, 2);
  twn->SetFaction(faction);
  return 0;
}

static int l_getCreatureCost(lua_State *L) {
  int creature = (int)luaL_checknumber(L, 1);
  int cost[NUM_RESOURCES];
  GetMonsterCost(creature, cost);
  for (int i = 0; i < NUM_RESOURCES; i++) {
    lua_pushinteger(L, cost[i]);
  }
  return 7;
}

static int l_getTownOwner(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
		if (twn->ownerIdx == -1)
						lua_pushnil(L);
		else
						deepbound_push(L, deepbind<playerData*>(&gpGame->players[twn->ownerIdx]));
  return 1;
}

static int l_setTownOwner(lua_State *L) {
		town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
		if (lua_isnil(L, 2))
		{
						lua_pop(L, 2);
						gpGame->ClaimTown(twn->idx, -1, 0);
		}
		else
		{
						playerData *player = (playerData*)GetPointerFromLuaClassTable(L, StackIndexOfArg(2, 2));
						gpGame->ClaimTown(twn->idx, GetPlayerNumber(player), 0);
		}
  return 0;
}

static int l_getTownX(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, twn->x);
  return 1;
}

static int l_getTownY(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, twn->y);
  return 1;
}

static int l_getTownIDFromPos(lua_State *L) {
  int x = (int)luaL_checknumber(L, 1);
  int y = (int)luaL_checknumber(L, 2);
  lua_pushinteger(L, gpGame->GetTownId(x, y));
  return 1;
}

static int l_getNumberOfCreatures(lua_State *L) {
				town* cstle = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
				int dwelling = (int)luaL_checknumber(L, 2);
				lua_pushinteger(L, cstle->numCreaturesInDwelling[dwelling]);
				return 1;
}

static int l_setNumberOfCreatures(lua_State *L) {
  town* cstle = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
  int dwllng = (int)luaL_checknumber(L, 2);
  int numcrtrs = (int)luaL_checknumber(L, 3);
  cstle->numCreaturesInDwelling[dwllng] = numcrtrs;
  return 0;
}

static int l_setNumGuildSpells(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
  int l = (int)luaL_checknumber(L, 2);
  int n = (int)luaL_checknumber(L, 3);
  twn->SetNumSpellsOfLevel(l, n);
  twn->GiveSpells(NULL);
  return 0;
}

static int l_setGuildSpell(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 4));
  int l = (int)luaL_checknumber(L, 2);
  int n = (int)luaL_checknumber(L, 3);
  int s = (int)luaL_checknumber(L, 4);
  twn->mageGuildSpells[l][n] = s;
  twn->GiveSpells(NULL);
  return 0;
}

static int l_getGuildSpell(lua_State *L) {
  town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
  int l = (int)luaL_checknumber(L, 2);
  int n = (int)luaL_checknumber(L, 3);
  int s = twn->mageGuildSpells[l][n];
  lua_pushinteger(L, s);
  return 1;
}

static int l_getBuildingFlag(lua_State *L) {
				town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
				bool yesno = twn->GetBuildingFlag();
				lua_pushboolean(L, yesno);
				return 1;
}

static int l_setBuildingFlag(lua_State *L) {
				town* twn = (town*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
				bool yesno = CheckBoolean(L, 2);
				twn->SetBuildingFlag(yesno);
				return 0;
}

static void register_town_funcs(lua_State *L) {
  lua_register(L, "GetCurrentTown", l_getCurrentTown);
  lua_register(L, "HasVisitingHero", l_hasVisitingHero);
  lua_register(L, "GetVisitingHero", l_getVisitingHero);
  lua_register(L, "BuildInCurrentTown", l_buildInCurrentTown);
  lua_register(L, "GetTown", l_getTown);
  lua_register(L, "GetTownName", l_getTownName);
  lua_register(L, "SetTownName", l_setTownName);
  lua_register(L, "GetTownByName", l_getTownByName);
  lua_register(L, "GetPlayerTown", l_getPlayerTown);
  lua_register(L, "BuildInTown", l_buildInTown);
  lua_register(L, "GetTownFaction", l_getTownFaction);
  lua_register(L, "SetTownFaction", l_setTownFaction);
  lua_register(L, "GetCreatureCost", l_getCreatureCost);
  lua_register(L, "GetTownOwner", l_getTownOwner);
  lua_register(L, "SetTownOwner", l_setTownOwner);
  lua_register(L, "GetTownX", l_getTownX);
  lua_register(L, "GetTownY", l_getTownY);
  lua_register(L, "GetTownIdFromPos", l_getTownIDFromPos);
		lua_register(L, "GetNumberOfCreatures", l_getNumberOfCreatures);
  lua_register(L, "SetNumberOfCreatures", l_setNumberOfCreatures);
  lua_register(L, "SetNumGuildSpells", l_setNumGuildSpells);
  lua_register(L, "SetGuildSpell", l_setGuildSpell);
  lua_register(L, "GetGuildSpell", l_getGuildSpell);
		lua_register(L, "GetBuildingFlag", l_getBuildingFlag);
		lua_register(L, "SetBuildingFlag", l_setBuildingFlag);
}

/************************************* Battle ******************************************/

// FIXME: DraggonFantasy's version used getFirstEmptyHex. That was more useful
static int l_battleSummonCreature(lua_State *L) {
  int side = (int)luaL_checknumber(L, 1);
  int hex = (int)luaL_checknumber(L, 2);
  int creature = (int)luaL_checknumber(L, 3);
  int quantity = (int)luaL_checknumber(L, 4);

  gpCombatManager->AddArmy(side, creature, quantity, hex, 0, 1);
  return 0;
}

static int l_isHexEmpty(lua_State *L) {
  int hexno = (int)luaL_checknumber(L, 1);
  if (!ValidHex(hexno)) {
    lua_pushboolean(L, 0);
  }
  else {
    hexcell *cell = &gpCombatManager->combatGrid[hexno];
    lua_pushboolean(L, cell->unitOwner == -1 && !cell->isBlocked);
  }

  return 1;
}

static int l_battleHasHero(lua_State *L) {
  int side = (int)luaL_checknumber(L, 1);
  lua_pushboolean(L, gpCombatManager->heroes[side] != NULL);
  return 1;
}

static int l_battleGetHero(lua_State *L) {
  int side = (int)luaL_checknumber(L, 1);
  deepbound_push(L, deepbind<hero*>(gpCombatManager->heroes[side]));
  return 1;
}

static int l_battleMessage(lua_State *L) {
  char* message = (char*)luaL_checkstring(L, 1);
  gpCombatManager->CombatMessage(message);
  return 0;
}

static int l_battleGetNumStacks(lua_State *L) {
  int side = (int)luaL_checknumber(L, 1);
  lua_pushinteger(L, gpCombatManager->numCreatures[side]);
  return 1;
}

static int l_battleGetStack(lua_State *L) {
  int side = (int)luaL_checknumber(L, 1);
  int idx = (int)luaL_checknumber(L, 2);
  deepbound_push(L, deepbind<army*>(&gpCombatManager->creatures[side][idx]));
  return 1;
}

static int l_getStackSide(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, creat->owningSide);
  return 1;
}

static int l_getStackType(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, creat->creatureIdx);
  return 1;
}

static int l_getStackQuantity(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, creat->quantity);
  return 1;
}

static int l_setStackQuantity(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int quantity = (int)luaL_checknumber(L, 2);
  creat->quantity = quantity;
  return 0;
}

static int l_getStackInitialQuantity(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, creat->initialQuantity);
  return 1;
}

static int l_setStackInitialQuantity(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int initialQuantity = (int)luaL_checknumber(L, 2);
  creat->initialQuantity = initialQuantity;
  return 0;
}

static int l_getStackHex(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, creat->occupiedHex);
  return 1;
}

static int l_getStackMorale(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, creat->morale);
  return 1;
}

static int l_setStackMorale(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int morale = (int)luaL_checknumber(L, 2);
  creat->morale = morale;
  return 0;
}

static int l_getStackLuck(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, creat->luck);
  return 1;
}

static int l_setStackLuck(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int luck = (int)luaL_checknumber(L, 2);
  creat->luck = luck;
  return 0;
}

static int l_getStackAttack(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, creat->creature.attack);
  return 1;
}

static int l_setStackAttack(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int attack = (int)luaL_checknumber(L, 2);
  creat->creature.attack = attack;
  return 0;
}

static int l_getStackDefense(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, creat->creature.defense);
  return 1;
}

static int l_setStackDefense(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int defense = (int)luaL_checknumber(L, 2);
  creat->creature.defense = defense;
  return 0;
}

static int l_getStackSpeed(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, creat->creature.speed);
  return 1;
}

static int l_setStackSpeed(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int speed = (int)luaL_checknumber(L, 2);
  creat->creature.speed = speed;
  return 0;
}

static int l_getStackShots(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, creat->creature.shots);
  return 1;
}

static int l_setStackShots(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int shots = (int)luaL_checknumber(L, 2);
  creat->creature.shots = shots;
  return 0;
}

static int l_getStackHp(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 1));
  lua_pushinteger(L, creat->creature.hp - creat->damage);
  return 1;
}

static int l_setStackHp(lua_State *L) {
  army *creat = (army*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 2));
  int hp = (int)luaL_checknumber(L, 2);
  creat->damage = creat->creature.hp - hp;
  return 0;
}

static int l_battleCastSpell(lua_State *L) {
				int spell = (int)luaL_checknumber(L, 1);
				int hex = (int)luaL_checknumber(L, 2);
				int effect = (int)luaL_checknumber(L, 3);
				int extra = (int)luaL_checknumber(L, 4);
				int side = (int)luaL_checknumber(L, 5);
				gpCombatManager->CastSpell(spell, hex, effect, extra, side);
				return 0;
}

static int l_getHeroCastThisTurn(lua_State *L)
{
    int side = (int)luaL_checknumber(L, 1);
    lua_pushinteger(L, gpCombatManager->heroCastThisTurn[side]);
    return 1;
}

static int l_setHeroCastThisTurn(lua_State *L)
{
    int side = (int)luaL_checknumber(L, 1);
    bool yesno = CheckBoolean(L, 2);
    if (yesno)
        gpCombatManager->heroCastThisTurn[side] = 1;
    else
        gpCombatManager->heroCastThisTurn[side] = 0;
    return 0;
}

static void register_battle_funcs(lua_State *L) {
  lua_register(L, "BattleSummonCreature", l_battleSummonCreature);
  lua_register(L, "IsHexEmpty", l_isHexEmpty);
  lua_register(L, "BattleHasHero", l_battleHasHero);
  lua_register(L, "BattleGetHero", l_battleGetHero);
  lua_register(L, "BattleMessage", l_battleMessage);
  lua_register(L, "BattleNumStacksForSide", l_battleGetNumStacks);
  lua_register(L, "BattleGetStack", l_battleGetStack);
  lua_register(L, "GetStackSide", l_getStackSide);
  lua_register(L, "GetStackType", l_getStackType);
  lua_register(L, "GetStackQuantity", l_getStackQuantity);
  lua_register(L, "SetStackQuantity", l_setStackQuantity);
  lua_register(L, "GetStackInitialQuantity", l_getStackInitialQuantity);
  lua_register(L, "SetStackInitialQuantity", l_setStackInitialQuantity);
  lua_register(L, "GetStackHex", l_getStackHex);
  lua_register(L, "GetStackMorale", l_getStackMorale);
  lua_register(L, "SetStackMorale", l_setStackMorale);
  lua_register(L, "GetStackLuck", l_getStackLuck);
  lua_register(L, "SetStackLuck", l_setStackLuck);
  lua_register(L, "GetStackAttack", l_getStackAttack);
  lua_register(L, "SetStackAttack", l_setStackAttack);
  lua_register(L, "GetStackDefense", l_getStackDefense);
  lua_register(L, "SetStackDefense", l_setStackDefense);
  lua_register(L, "GetStackSpeed", l_getStackSpeed);
  lua_register(L, "SetStackSpeed", l_setStackSpeed);
  lua_register(L, "GetStackShots", l_getStackShots);
  lua_register(L, "SetStackShots", l_setStackShots);
  lua_register(L, "GetStackHp", l_getStackHp);
  lua_register(L, "SetStackHp", l_setStackHp);
		lua_register(L, "BattleCastSpell", l_battleCastSpell);
  lua_register(L, "GetHeroCastThisTurn", l_getHeroCastThisTurn);
  lua_register(L, "SetHeroCastThisTurn", l_setHeroCastThisTurn);
}

/**************************************** Campaign *********************************************/

static int l_getCampaignChoiceType(lua_State *L) {
  int curMapID = xCampaign.currentMapID;
  SCampaignChoice *curChoice = &xCampaignChoices[xCampaign.campaignID][curMapID][xCampaign.bonusChoices[curMapID]];
  lua_pushinteger(L, curChoice->type);
	return 1;
}

static int l_getCampaignChoiceField(lua_State *L) {
  int curMapID = xCampaign.currentMapID;
  SCampaignChoice *curChoice = &xCampaignChoices[xCampaign.campaignID][curMapID][xCampaign.bonusChoices[curMapID]];
  lua_pushinteger(L, curChoice->field);
	return 1;
}

static int l_getCampaignChoiceAmount(lua_State *L) {
  int curMapID = xCampaign.currentMapID;
  SCampaignChoice *curChoice = &xCampaignChoices[xCampaign.campaignID][curMapID][xCampaign.bonusChoices[curMapID]];
  lua_pushinteger(L, curChoice->amount);
	return 1;
}

static int l_getCampaignChoice(lua_State *L) {
  int curMapID = xCampaign.currentMapID;
  SCampaignChoice *curChoice = &xCampaignChoices[xCampaign.campaignID][curMapID][xCampaign.bonusChoices[curMapID]];
  deepbound_push(L, deepbind<SCampaignChoice*>(curChoice));
	return 1;
}

static void register_campaign_funcs(lua_State *L) {
  lua_register(L, "GetCampaignChoiceType", l_getCampaignChoiceType);
  lua_register(L, "GetCampaignChoiceField", l_getCampaignChoiceField);
  lua_register(L, "GetCampaignChoiceAmount", l_getCampaignChoiceAmount);
  lua_register(L, "GetCampaignChoice", l_getCampaignChoice);
}

/************************************** Uncategorized ******************************************/

static int l_playsoundeffect(lua_State *L) {
	std::string snd = std::string(luaL_checkstring(L, 1));
	PlaySoundEffect(snd, SND_DO_WAIT, NULL);
	return 0;
}

static int l_getinclinedtojoin(lua_State *L) {
	int x = (int)luaL_checknumber(L, 1);
	int y = (int)luaL_checknumber(L, 2);
	int inclinedToJoin = 0;
	if ((x >= 0) && (y >= 0) && (x < gpGame->map.width) && (y < gpGame->map.height)) {
		int cellIdx = y * gpGame->map.height + x;
		if (gpGame->map.tiles[cellIdx].objType == (LOCATION_ARMY_CAMP | TILE_HAS_EVENT)) {
			// This uses the correct WillJoin bit, but does not mean that the army at the map cell here will not fight a hero (see "l_setinclinedtojoin").
			inclinedToJoin = (gpGame->map.tiles[cellIdx].extraInfo & (1 << 12));
		}
	}
	if (inclinedToJoin) {
		lua_pushinteger(L, 1);
	} else {
		lua_pushinteger(L, 0);
	}
	return 1;
}

static int l_setinclinedtojoin(lua_State *L) {
	int x = (int)luaL_checknumber(L, 1);
	int y = (int)luaL_checknumber(L, 2);
	bool inclinedToJoin = CheckBoolean(L, 3);
	if ((x >= 0) && (y >= 0) && (x < gpGame->map.width) && (y < gpGame->map.height)) {
		int cellIdx = y * gpGame->map.height + x;
		if (gpGame->map.tiles[cellIdx].objType == (LOCATION_ARMY_CAMP | TILE_HAS_EVENT)) {
			// These are using the correct WillJoin bit, but it is not guaranteed to make every army join any hero no matter what.
			// This condition also depends on the ratio of the hero's power to the army's power, among other variables.
			if (inclinedToJoin) {
				gpGame->map.tiles[cellIdx].extraInfo |= (1 << 12);
			} else {
				gpGame->map.tiles[cellIdx].extraInfo &= ~(1 << 12);
			}
		}
	}
	return 0;
}

static int l_startbattle(lua_State *L) {
  hero* hro = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 4));
  int mon1 = (int)luaL_checknumber(L, 2);
  int mon1quantity = (int)luaL_checknumber(L, 3);
  int switchSides = (int)luaL_checknumber(L, 4);
  mapCell* mapcell = gpAdvManager->GetCell(hro->x, hro->y);
  int winningSide = gpAdvManager->CombatMonsterEvent(hro, mon1, mon1quantity, mapcell, hro->x, hro->y, switchSides, hro->x, hro->y, -1, 0, 0, -1, 0, 0);
  lua_pushinteger(L, winningSide);
  return 1;
}

static int l_toggleAIArmySharing(lua_State *L) {
  bool toggle = CheckBoolean(L, 1);
  gpGame->allowAIArmySharing = toggle;
  return 0;
}

static int l_getSpellLevel(lua_State *L) {
				int spell = luaL_checknumber(L, 1);
				lua_pushinteger(L, gsSpellInfo[spell].level);
				return 1;
}

static int l_getSpellName(lua_State *L) {
				int spell = luaL_checknumber(L, 1);
				lua_pushstring(L, gSpellNames[spell]);
				return 1;
}

static int l_getSpellManaCost(lua_State *L) {
				int spell = luaL_checknumber(L, 1);
				lua_pushinteger(L, gsSpellInfo[spell].cost);
				return 1;
}

static int l_setSpellManaCost(lua_State *L) {
				int spell = luaL_checknumber(L, 1);
				int cost = luaL_checknumber(L, 2);
				gsSpellInfo[spell].cost = cost;
				return 0;
}

static int l_getArtifactName(lua_State *L) {
				int artifact = luaL_checknumber(L, 1);
				lua_pushstring(L, GetArtifactName(artifact).c_str());
				return 1;
}

static int l_getUltimateArtifactX(lua_State *L) {
				lua_pushinteger(L, gpGame->ultimateArtifactLocX);
				return 1;
}

static int l_getUltimateArtifactY(lua_State *L) {
				lua_pushinteger(L, gpGame->ultimateArtifactLocY);
				return 1;
}

static int l_getUltimateArtifact(lua_State *L) {
				lua_pushinteger(L, gpGame->ultimateArtifactIdx);
				return 1;
}

static int l_setUltimateArtifact(lua_State *L) {
				int artifact = luaL_checknumber(L, 1);
    int x = luaL_checknumber(L, 2);
    int y = luaL_checknumber(L, 3);
    gpGame->ultimateArtifactLocX = x;
    gpGame->ultimateArtifactLocY = y;
				gpGame->ultimateArtifactIdx = artifact;
				return 0;
}

static int l_forceComputerPlayerChase(lua_State *L) {
	hero* src = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(1, 3));
	hero* dst = (hero*)GetPointerFromLuaClassTable(L, StackIndexOfArg(2, 3));
	bool force = CheckBoolean(L, 3);
	gpGame->ForceComputerPlayerChase(src, dst, force);
	return 0;
}

static void register_uncategorized_funcs(lua_State *L) {
  lua_register(L, "PlaySoundEffect", l_playsoundeffect);
  lua_register(L, "GetInclinedToJoin", l_getinclinedtojoin);
  lua_register(L, "SetInclinedToJoin", l_setinclinedtojoin);
  lua_register(L, "StartBattle", l_startbattle);
  lua_register(L, "ToggleAIArmySharing", l_toggleAIArmySharing);
		lua_register(L, "GetSpellLevel", l_getSpellLevel);
		lua_register(L, "GetSpellName", l_getSpellName);
		lua_register(L, "GetSpellManaCost", l_getSpellManaCost);
		lua_register(L, "SetSpellManaCost", l_setSpellManaCost);
		lua_register(L, "GetArtifactName", l_getArtifactName);
		lua_register(L, "GetUltimateArtifactX", l_getUltimateArtifactX);
		lua_register(L, "GetUltimateArtifactY", l_getUltimateArtifactY);
		lua_register(L, "GetUltimateArtifact", l_getUltimateArtifact);
		lua_register(L, "SetUltimateArtifact", l_setUltimateArtifact);
  lua_register(L, "ForceComputerPlayerChase", l_forceComputerPlayerChase);
}

/****************************************************************************************************************/


void set_scripting_funcs(lua_State *L) {
  register_dialog_funcs(L);
  register_date_funcs(L);
  register_player_funcs(L);
  register_hero_funcs(L);
  register_map_funcs(L);
  register_town_funcs(L);
  register_battle_funcs(L);
  register_campaign_funcs(L);
  register_uncategorized_funcs(L);
}
