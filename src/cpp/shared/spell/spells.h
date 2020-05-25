#ifndef SPELLS_H
#define SPELLS_H

#include "adventure/adv.h"
#include "spell/spell_constants.h"

#pragma pack(push, 1)

struct SSpellInfo {
  char soundName[9];
  char level;
  char magicBookIconIdx;
  char creatureEffectAnimationIdx; // gCombatFxNames
  __int16 appearingChance;
  char cost;
  char nonMagicFactionAppearanceChance;
  int field_10;
  char field_14;
  char attributes;
};

SSpellInfo gsSpellInfo[];

enum Spell : int {
  SPELL_FIREBALL = 0,
  SPELL_FIREBLAST = 1,
  SPELL_LIGHTNING_BOLT = 2,
  SPELL_CHAIN_LIGHTNING = 3,
  SPELL_TELEPORT = 4,
  SPELL_CURE = 5,
  SPELL_MASS_CURE = 6,
  SPELL_RESURRECT = 7,
  SPELL_RESURRECT_TRUE = 8,
  SPELL_HASTE = 9,
  SPELL_MASS_HASTE = 10,
  SPELL_SLOW = 11,
  SPELL_MASS_SLOW = 12,
  SPELL_BLIND = 13,
  SPELL_BLESS = 14,
  SPELL_MASS_BLESS = 15,
  SPELL_STONESKIN = 16,
  SPELL_STEELSKIN = 17,
  SPELL_CURSE = 18,
  SPELL_MASS_CURSE = 19,
  SPELL_HOLY_WORD = 20,
  SPELL_HOLY_SHOUT = 21,
  SPELL_ANTI_MAGIC = 22,
  SPELL_DISPEL_MAGIC = 23,
  SPELL_MASS_DISPEL = 24,
  SPELL_MAGIC_ARROW = 25,
  SPELL_BERZERKER = 26,
  SPELL_ARMAGEDDON = 27,
  SPELL_ELEMENTAL_STORM = 28,
  SPELL_METEOR_SHOWER = 29,
  SPELL_PARALYZE = 30,
  SPELL_HYPNOTIZE = 31,
  SPELL_COLD_RAY = 32,
  SPELL_COLD_RING = 33,
  SPELL_DISRUPTING_RAY = 34,
  SPELL_DEATH_RIPPLE = 35,
  SPELL_DEATH_WAVE = 36,
  SPELL_DRAGON_SLAYER = 37,
  SPELL_BLOOD_LUST = 38,
  SPELL_ANIMATE_DEAD = 39,
  SPELL_MIRROR_IMAGE = 40,
  SPELL_SHIELD = 41,
  SPELL_MASS_SHIELD = 42,
  SPELL_SUMMON_EARTH_ELEMENTAL = 43,
  SPELL_SUMMON_AIR_ELEMENTAL = 44,
  SPELL_SUMMON_FIRE_ELEMENTAL = 45,
  SPELL_SUMMON_WATER_ELEMENTAL = 46,
  SPELL_EARTHQUAKE = 47,
  SPELL_VIEW_MINES = 48,
  SPELL_VIEW_RESOURCES = 49,
  SPELL_VIEW_ARTIFACTS = 50,
  SPELL_VIEW_TOWNS = 51,
  SPELL_VIEW_HEROES = 52,
  SPELL_VIEW_ALL = 53,
  SPELL_IDENTIFY = 54,
  SPELL_SUMMON_BOAT = 55,
  SPELL_DIMENSION_DOOR = 56,
  SPELL_TOWN_GATE = 57,
  SPELL_TOWN_PORTAL = 58,
  SPELL_VISIONS = 59,
  SPELL_HAUNT = 60,
  SPELL_SET_EARTH_GUARDIAN = 61,
  SPELL_SET_AIR_GUARDIAN = 62,
  SPELL_SET_FIRE_GUARDIAN = 63,
  SPELL_SET_WATER_GUARDIAN = 64,

  SPELL_MEDUSA_PETRIFY = 101,
  SPELL_ARCHMAGI_DISPEL = 102,
  SPELL_NONE = -1,

  SPELL_AWARENESS = 65,
  SPELL_SHADOW_MARK = 66,
  SPELL_MARKSMAN_PIERCE = 67,
  SPELL_PLASMA_CONE = 68,
  SPELL_FORCE_SHIELD = 69,
  SPELL_MASS_FORCE_SHIELD = 70,
  SPELL_FIRE_BOMB = 71,
  SPELL_IMPLOSION_GRENADE = 72,
		SPELL_DISENCHANT = 73,
		SPELL_MASS_DISENCHANT = 74,
};


enum SPELL_ATTRIBUTES {
  ATTR_COMMON_SPELL = 0x1,
  ATTR_COMBAT_SPELL = 0x2,
  ATTR_ADVENTURE_SPELL = 0x4,
  ATTR_DURATIONED_SPELL = 0x8,
};


enum SPELL_CATEGORY {
  SPELL_CATEGORY_COMBAT = 0,
  SPELL_CATEGORY_ADVENTURE = 1,
  SPELL_CATEGORY_ALL = 2,
};

enum STACK_MODIFYING_EFFECT {
  EFFECT_HASTE = 0,
  EFFECT_SLOW = 1,
  EFFECT_BLIND = 2,
  EFFECT_BLESS = 3,
  EFFECT_CURSE = 4,
  EFFECT_BERSERKER = 5,
  EFFECT_PARALYZE = 6,
  EFFECT_HYPNOTIZE = 7,
  EFFECT_DRAGON_SLAYER = 8,
  EFFECT_BLOOD_LUST = 9,
  EFFECT_SHIELD = 10,
  EFFECT_PETRIFY = 11,
  EFFECT_ANTI_MAGIC = 12,
  EFFECT_STONESKIN = 13,
  EFFECT_STEELSKIN = 14,
  EFFECT_BURN = 15,
  EFFECT_SHADOW_MARK = 16,
  EFFECT_DAZE = 17,
  EFFECT_FORCE_SHIELD = 18
};

enum CREATURE_EFFECT_ANIMATION_INDEX {
  ANIM_NONE_IDX = 0,
  ANIM_MAGIC01_IDX = 1,
  ANIM_MAGIC02_IDX = 2,
  ANIM_MAGIC03_IDX = 3,
  ANIM_MAGIC04_IDX = 4,
  ANIM_MAGIC05_IDX = 5,
  ANIM_MAGIC06_IDX = 6,
  ANIM_MAGIC07_IDX = 7,
  ANIM_MAGIC08_IDX = 8,
  ANIM_RAINBOW_LUCK_IDX = 9,
  ANIM_CLOUD_LUCK_IDX = 10,
  ANIM_MORALE_GOOD_IDX = 11,
  ANIM_MORALE_BAD_IDX = 12,
  ANIM_RED_DEATH_IDX = 13,
  ANIM_RED_FIRE_IDX = 14,
  ANIM_SPARKS_IDX = 15,
  ANIM_ELECTRIC_IDX = 16,
  ANIM_PHISICAL_IDX = 17,
  ANIM_BLUEFIRE_IDX = 18,
  ANIM_ICECLOUD_IDX = 19,
  ANIM_LICHCLOUD_IDX = 20,
  ANIM_BLESS_IDX = 21,
  ANIM_BERZERK_IDX = 22,
  ANIM_SHIELD_IDX = 23,
  ANIM_HASTE_IDX = 24,
  ANIM_PARALYZE_IDX = 25,
  ANIM_HYPNOTIZE_IDX = 26,
  ANIM_DRAGONSLAYER_IDX = 27,
  ANIM_BLIND_IDX = 28,
  ANIM_CURSE_IDX = 29,
  ANIM_STONESKIN_IDX = 30,
  ANIM_STEELSKIN_IDX = 31,
};

int __fastcall GetManaCost(int spell, hero* hro);
int __fastcall GetManaCost_orig(int spell, hero* hro);
int GetManaCost(int spell);

#pragma pack(pop)

#endif