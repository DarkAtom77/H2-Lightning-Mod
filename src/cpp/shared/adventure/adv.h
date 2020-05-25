#ifndef ADV_H
#define ADV_H

#include "gui/gui.h"
#include "resource/resources.h"

#include "base.h"
#include "skills.h"

class mapCell;
extern class town;
extern class fullMap;

#define ORIG_SPELLS 65

#pragma pack(push, 1)

#define MAX_TOTAL_HEROES 48

#define CREATURES_IN_ARMY 5

#define NUM_RESOURCES 7
#define NUM_SECONDARY_RESOURCES 6

extern unsigned char PlayerVisitedObject[144][144];

enum RESOURCES {
  RESOURCE_WOOD = 0,
  RESOURCE_MERCURY = 1,
  RESOURCE_ORE = 2,
  RESOURCE_SULFUR = 3,
  RESOURCE_CRYSTAL = 4,
  RESOURCE_GEMS = 5,
  RESOURCE_GOLD = 6,
};

enum HERO_PORTRAITS {
				//knight
				PORTRAIT_LORD_KILBURN = 0,
				PORTRAIT_SIR_GALLANT = 1,
				PORTRAIT_ECTOR = 2,
				PORTRAIT_GWENNETH = 3,
				PORTRAIT_TYRO = 4,
				PORTRAIT_AMBROSE = 5,
				PORTRAIT_RUBY = 6,
				PORTRAIT_MAXIMUS = 7,
				PORTRAIT_DIMITRI = 8,
				//barbarian
				PORTRAIT_THUNDAX = 9,
				PORTRAIT_FINEOUS = 10,
				PORTRAIT_JOJOSH = 11,
				PORTRAIT_CRAG_HACK = 12,
				PORTRAIT_JEZEBEL = 13,
				PORTRAIT_JACLYN = 14,
				PORTRAIT_ERGON = 15,
				PORTRAIT_TSABU = 16,
				PORTRAIT_ATLAS = 17,
				//sorceress
				PORTRAIT_ASTRA = 18,
				PORTRAIT_NATASHA = 19,
				PORTRAIT_TROYAN = 20,
				PORTRAIT_VATAWNA = 21,
				PORTRAIT_REBECCA = 22,
				PORTRAIT_GEM = 23,
				PORTRAIT_ARIEL = 24,
				PORTRAIT_CARLAWN = 25,
				PORTRAIT_LUNA = 26,
				//warlock
				PORTRAIT_ARIE = 27,
				PORTRAIT_ALAMAR = 28,
				PORTRAIT_VESPER = 29,
				PORTRAIT_CRODO = 30,
				PORTRAIT_BAROK = 31,
				PORTRAIT_KASTORE = 32,
				PORTRAIT_AGAR = 33,
				PORTRAIT_FALAGAR = 34,
				PORTRAIT_WRATHMONT = 35,
				//wizard
				PORTRAIT_MYRA = 36,
				PORTRAIT_FLINT = 37,
				PORTRAIT_DAWN = 38,
				PORTRAIT_HALON = 39,
				PORTRAIT_MYRINI = 40,
				PORTRAIT_WILFREY = 41,
				PORTRAIT_SARAKIN = 42,
				PORTRAIT_KALINDRA = 43,
				PORTRAIT_MANDIGAL = 44,
				//necromancer
				PORTRAIT_ZOM = 45,
				PORTRAIT_DARLANA = 46,
				PORTRAIT_ZAM = 47,
				PORTRAIT_RANLOO = 48,
				PORTRAIT_CHARITY = 49,
				PORTRAIT_RIALDO = 50,
				PORTRAIT_ROXANA = 51,
				PORTRAIT_SANDRO = 52,
				PORTRAIT_CELIA = 53,
				//campaign
				PORTRAIT_ROLAND = 54,
				PORTRAIT_CORLAGON = 55,
				PORTRAIT_SISTER_ELIZA = 56,
				PORTRAIT_ARCHIBALD = 57,
				PORTRAIT_HALTON = 58,
				PORTRAIT_BROTHER_BRAX = 59,
				//expansion
				PORTRAIT_SOLMYR = 60,
				PORTRAIT_KRAEGER = 61,
				PORTRAIT_IBN_FADLAN = 62,
				PORTRAIT_UNCLE_IVAN = 63,
				PORTRAIT_JOSEPH = 64,
				PORTRAIT_GALLAVANT = 65,
				PORTRAIT_JARKONAS_VI = 66,
				PORTRAIT_ALBERON = 67,
				PORTRAIT_DRAKONIA = 68,
				PORTRAIT_MARTINE = 69,
				PORTRAIT_JARKONAS = 70,
				//ironfist
				PORTRAIT_LORD_VARUUN = 71,
				PORTRAIT_JOHN_CARVER = 72,
				//captains
				PORTRAIT_CAPTAIN_KNIGHT = 90,
				PORTRAIT_CAPTAIN_BARBARIAN = 91,
				PORTRAIT_CAPTAIN_SORCERESS = 92,
				PORTRAIT_CAPTAIN_WARLOCK = 93,
				PORTRAIT_CAPTAIN_WIZARD = 94,
				PORTRAIT_CAPTAIN_NECROMANCER = 95,
};

class hero;

class armyGroup {
public:
  char creatureTypes[CREATURES_IN_ARMY];
  __int16 quantities[CREATURES_IN_ARMY];

  armyGroup() {
    for (int i = 0; i < ELEMENTS_IN(this->creatureTypes); i++) {
      this->creatureTypes[i] = -1;
    }

    for (int i = 0; i < ELEMENTS_IN(this->quantities); i++) {
      this->quantities[i] = 0;
    }
  };

  int Add(int, int, int);
  void ClearArmy();
		int __thiscall CanJoin(int creatureType);
  int GetMorale(hero *hro, town *twn, armyGroup *armyGr);
  int GetMorale_orig(hero *hro, town *twn, armyGroup *armyGr);
  int HasAllUndead();
  int HasSomeUndead();
  int IsHomogeneous(int a2);
};

class hero {
public:
  __int16 spellpoints;
  char idx;
  char ownerIdx;
  char field_4;
  char field_5;
  char field_6;
  char field_7;
  char field_8;
  char field_9;
  char name[13];
  char factionID;
  char heroID;
  int x;
  int y;
  __int16 field_21;
  __int16 field_23;
  __int16 field_25;
  __int16 field_27;
  __int8 relatedToX;
  __int8 relatedToY;
  __int8 relatedToFactionID;
  __int8 directionFacing;
  __int16 occupiedObjType;
  __int16 occupiedObjVal;
  int mobility;
  int remainingMobility;
  int experience;
  __int16 oldLevel;
  char primarySkills[NUM_PRIMARY_SKILLS];
  char field_43;
  char tempMoraleBonuses;
  char tempLuckBonuses;
  char field_46;
  int gazeboesVisited;
  int fortsVisited;
  int witchDoctorHutsVisited;
  int mercenaryCampsVisited;
  int standingStonesVisited;
  int treesOfKnowledgeVisited;
  int xanadusVisited;
  char randomSeed;
  char wisdomLastOffered;
  armyGroup army;
  char secondarySkillLevel[14];
  char skillIndex[14];
  int numSecSkillsKnown;

  /*
   * In order to put more space in the hero class for new spells,
   * we would need to decompile virtually the entire codebase, as doing so would also
   * change the size of the game class.
   *
   * We instead replace the statically-allocated spellsLearned array with a pointer to a dynamically-allocated one,
   * so that we only need to decompile the direct accesses.
   *
   * There ARE memory leaks associated with doing this. We've done much of what we can to avoid this,
   * but there's no easy way out, and the leak is upper-bounded by 3 kb every time you load a map
   */
   //char spellsLearned[65];
  char* spellsLearned;
  char _[ORIG_SPELLS - sizeof(char*)];

#define FIELD_AFTER_SPELLS_LEARNED artifacts
  char artifacts[14];
  int flags;
  char isCaptain;
  int field_E8;
#define LAST_SW_HERO_FIELD scrollSpell
  char scrollSpell[14];

  hero();
  ~hero(); //newly added
  void Deallocate(int);
  void AddSpell(int);
  void AddSpell(int, int);
  int HasSpell(int);
  int GetNumSpells(int);
  int GetNthSpell(int, int);
  void UseSpell(int);

  int HasArtifact(int);
  void TakeArtifact(int);
  int CountEmptyArtifactSlots();
  int CountEmptyCreatureSlots();
  signed char Stats(int);
  signed char GetSSLevel(int);
  void SetSS(int, int);
  void CheckLevel();
  void CheckLevel_orig();
  int GiveSS(int, int);
  void ClearSS();
  int CalcMobility();
  int CalcMobility_orig();

  void Read(int, signed char);
  void Write(int, signed char);
  void ResetSpellsLearned();

  void SetPrimarySkill(int, int);
  int GetLevel();
  void Clear();
  int NumArtifacts();
  town* GetOccupiedTown();
};

enum class Sex { Male, Female };

class HeroExtraII
{
public:
				HeroExtraII(hero* hro);
				HeroExtraII(hero* hro, Sex sex);
				HeroExtraII(hero& hro);
				HeroExtraII(hero& hro, Sex sex);
				void VisitArena(int x, int y, bool set);
				bool HasVisitedArena(int x, int y);
				Sex GetHeroSex();
				void ResetHeroSex(); //automatic by the portrait
				void SetHeroSex(Sex sex);
				hero& hro;
private:
				Sex sex;
				int specialty; //for the future, not used right now
				bool objectsVisited[144][144]; //vector<bool> is more space-effiecient, but this is a matrix :(
};

enum HERO_FLAGS {
  // 0x1
  HERO_BUOY_VISITED = 0x2,
  HERO_FOUNTAIN_VISITED = 0x4,
  HERO_OASIS_VISITED = 0x8,
  HERO_FAIRY_RING_VISITED = 0x10,
  HERO_GRAVEYARD_ROBBER = 0x20,
  HERO_SHIPWRECK_ROBBER = 0x40,
  HERO_AT_SEA = 0x80,
  HERO_TEMPLE_VISITED = 0x100,
  HERO_WATERING_HOLE_VISITED = 0x200,
  HERO_DERELICT_SHIP_ROBBER = 0x400,
  // 0x800
  HERO_WELL_VISITED = 0x1000,
  HERO_IDOL_VISITED = 0x2000,
  HERO_PYRAMID_RAIDED = 0x4000,
  HERO_ARMY_COMPACT = 0x8000,
  HERO_MERMAID_VISITED = 0x100000,
  HERO_SIRENS_VISITED = 0x200000,
  HERO_ARENA_VISITED = 0x400000,
  HERO_STABLE_VISITED = 0x800000
};

extern char cHeroTypeInitial[];
extern signed __int8 captainStats[][NUM_PRIMARY_SKILLS];

class mapCell;

class advManager : public baseManager {
public:
  int field_36;
  widget *someComponents[2][12];
  heroWindow *adventureScreen;
  int sizeOfSomethingMapRelated;
  int field_A2;
  int currentTerrain;
  int field_AA;
  fullMap *map;
  iconWidget *heroScrollbarKnob;
  iconWidget *castleScrollbarKnob;
  int field_BA;
  int field_BE;
  tileset *groundTileset;
  tileset *clofTileset;
  tileset *stonTileset;
  icon *tilesetIcns[64];
  icon *radarIcon;
  icon *clopIcon;
  int viewX;
  int viewY;
  int field_1DE;
  int field_1E2;
  int xOff;
  int yOff;
  int field_1EE;
  int field_1F2;
  int mapPortLeftX;
  int mapPortTopY;
  int field_1FE;
  int field_202;
  int field_206;
  int field_20A;
  int field_20E;
  int field_212;
  int field_216;
  icon *heroIcons[6];
  icon *boatIcon;
  icon *frothIcon;
  icon *shadowIcon;
  icon *boatShadowIcon;
  icon *flagIconsHero[6];
  icon *flagIconsBoat[6];
  int field_272;
  int field_276;
  int mobilizedHeroFactionOrBoat;
  int mobilizedHeroDirection;
  int mobilizedHeroBaseFrameBit8IsFlip;
  int mobilizedHeroAnimPos;
  int mobilizedHeroCycle;
  int mobilizedHeroTurning;
  int field_292;
  int field_296;
  int field_29A;
  int field_29E;
  int hasDrawnCursor;
  int heroMobilized;
  int field_2AA;
  int field_2AE;
  int field_2B2;
  int field_2B6;
  int field_2BA;
  int field_2BE;
  int field_2C2[4][2];
  void *loopSamples[28];
  sample *walkSamples[9];
  int identifyCast;
  int field_37A;

  advManager();

  mapCell *GetCell(int x, int y);
  void DrawCell(int x, int y, int cellCol, int cellRow, int cellDrawingPhaseFlags, int a6);
  void DrawCell_orig(int x, int y, int cellCol, int cellRow, int cellDrawingPhaseFlags, int a6);
  void EraseObj(mapCell*, int x, int y);

  void PurgeMapChangeQueue();
  void CheckSetEvilInterface(int, int);

		void __thiscall HouseEvent(class hero *, class mapCell *);

		void __thiscall HouseEvent_orig(class hero *, class mapCell *);

  void DemobilizeCurrHero();

  void DimensionDoor();
  void TeleportTo(hero*, int, int, int, int);

  void CastSpell(int);
  void CastSpell_orig(int);

  void CompleteDraw(int);
  void RedrawAdvScreen(int, int);
  void UpdateRadar(int, int);
  void UpdateHeroLocator(int, int, int);
  void UpdBottomView(int, int, int);
  void EventSound(int locType, int locType2, SAMPLE2 *samp);

  void __thiscall EventWindow(int, int, char *, int, int, int, int, int);
  void __thiscall UpdateHeroLocators(int, int);
  void __thiscall UpdateScreen(int, int);
  void UpdateTownLocators(int a2, int updateScreen);

  int ProcessDeSelect(struct tag_message *GUIMessage_evt, int *a3, class mapCell **a4);
  int ProcessDeSelect_orig(struct tag_message *, int *, class mapCell **);

		int __thiscall ProcessSearch_orig(int, int);
		int __thiscall ProcessSearch(int, int);

  virtual int Open(int);

  void PasswordEvent(mapCell *tile, hero *hero);
  int BarrierEvent(mapCell *tile, hero *hero);

  void ExpansionRecruitEvent(class hero*, int, short*);

  mapCell* MoveHero(int, int, int *, int *, int *, int, int *, int);
  mapCell* MoveHero_orig(int, int, int *, int *, int *, int, int *, int);

  void DoEvent_orig(class mapCell *, int, int);
  void DoEvent(class mapCell *cell, int locX, int locY);
		void DoAIEvent_orig(class mapCell *, class hero *, int, int);
		void DoAIEvent(class mapCell * cell, class hero *hro, int locX, int locY);

  void HandleSpellShrine(class mapCell *cell, int LocationType, hero *hro, SAMPLE2 *res2, int locX, int locY);
  void HandlePyramid(class mapCell *cell, int LocationType, hero *hro, SAMPLE2 *res2, int locX, int locY);
		void HandleWitchHut(class mapCell *cell, int LocationType, hero *hro, SAMPLE2* res2, int locX, int locY);
		void HandleArena(class mapCell *cell, int LocationType, hero *hro, SAMPLE2* res2, int locX, int locY);
		void HandleAlchemistTower(class mapCell *cell, int locType, hero *hro, SAMPLE2 *res2, int locX, int locY);
		//Need a better way to structure the way to handle objects, otherwise, in the future, there will be about 100 lines of these HandleObject declarations

  int CombatMonsterEvent(class hero *hero, int mon1, int mon1quantity, class mapCell *mapcell, int locX, int locY, int switchSides, int locX2, int locY2, int mon2, int mon2quantity, int mon2stacks, int mon3, int mon3quantity, int mon3stacks);
  int MapPutArmy(int x, int y, int monIdx, int monQty);
		//int MapPutResource(int x, int y, int resIdx, int resQty);

  int GiveExperience(class hero*, int, int);

  void QuickInfo(int, int);
  void QuickInfo_orig(int, int);
		void ShrineQuickInfo(int xLoc, int yLoc);
		void WitchHutQuickInfo(int xLoc, int yLoc);
		void ArtifactQuickInfo(int xLoc, int yLoc);
		void ArenaQuickInfo(int xLoc, int yLoc, int TriggerX, int TriggerY);

  void PlayerMonsterInteract(mapCell *cell, mapCell *other, hero *player, int *window, int a1, int a2, int a3, int a4, int a5);
  void PlayerMonsterInteract_orig(mapCell *cell, mapCell *other, hero *player, int *window, int a1, int a2, int a3, int a4, int a5);

  void ComputerMonsterInteract(mapCell *cell, hero *computer, int *a1);
  void ComputerMonsterInteract_orig(mapCell *cell, hero *computer, int *a1);

  void TownQuickView(int a2, int a3, int a4, int a5);
  int IsCrystalBallInEffect(int x, int y, int a3);
  void SetTownContext(int townID);
  char * GetArmySizeName(signed int amt, int queryType);
  char * GetQuantityString(int thievesGuildsLevel, town* town, int garrisonIdx);
  void SetInitialMapOrigin();
  void DrawCursor();
  void DrawCursorShadow();
  int GetCloudLookup(int a1, int a2);
  int GetCursorBaseFrame(int direction);
  void ForceNewHover();
  void GetCursorSampleSet(int speed);
  void DisableButtons();
  void EnableButtons();
};

extern advManager* gpAdvManager;

extern int giMapChangeCtr;

extern heroWindow* heroWin;
extern int giHeroScreenSrcIndex;

extern char *gTilesetFiles[];
extern int giAdjacentMonsterUpperBoundX;
extern int giAdjacentMonsterUpperBoundY;
extern int giAdjacentMonsterX;
extern int giAdjacentMonsterY;
extern int giAdjacentMonsterLowerBoundX;
extern int giAdjacentMonsterLowerBoundY;

hero* GetCurrentHero();
bool GetMapCellXY(mapCell* cell, int* x, int* y);

int __fastcall GiveArtifact(hero*, int artifact, int checkEndGame, signed char scrollSpell);
void __fastcall GiveTakeArtifactStat(hero *h, int art, int take);
void __fastcall GiveTakeArtifactStat_orig(hero *h, int art, int take);
void __fastcall GetMonsterCost(int, int * const);

int GetShrineSpell(int x, int y);
void SetShrineSpell(int x, int y, int spell);
int GetWitchHutSkill(int x, int y);
void SetWitchHutSkill(int x, int y, int skill);
int GetWagonType(int x, int y);

#pragma pack(pop)

#endif
