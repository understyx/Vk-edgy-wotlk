/**
 * @file offsets.h
 * @brief WoW WotLK 3.3.5a memory offsets (organized with namespaces from IceFlake)
 */

#ifndef WOWMEMORY_OFFSETS_H
#define WOWMEMORY_OFFSETS_H

#include <cstdint>

namespace WoWDescriptors {

enum ObjectFields : uint32_t {
    OBJECT_FIELD_GUID = 0x0000, // Size: 2, Type: LONG, Flags: PUBLIC
    OBJECT_FIELD_TYPE = 0x0002, // Size: 1, Type: INT, Flags: PUBLIC
    OBJECT_FIELD_ENTRY = 0x0003, // Size: 1, Type: INT, Flags: PUBLIC
    OBJECT_FIELD_SCALE_X = 0x0004, // Size: 1, Type: FLOAT, Flags: PUBLIC
    OBJECT_FIELD_PADDING = 0x0005, // Size: 1, Type: INT, Flags: NONE
    OBJECT_END = 0x0006,
};

enum ItemFields : uint32_t {
    ITEM_FIELD_OWNER = OBJECT_END + 0x0000,
    ITEM_FIELD_CONTAINED = OBJECT_END + 0x0002,
    ITEM_FIELD_CREATOR = OBJECT_END + 0x0004,
    ITEM_FIELD_GIFTCREATOR = OBJECT_END + 0x0006,
    ITEM_FIELD_STACK_COUNT = OBJECT_END + 0x0008,
    ITEM_FIELD_DURATION = OBJECT_END + 0x0009,
    ITEM_FIELD_SPELL_CHARGES = OBJECT_END + 0x000A,
    ITEM_FIELD_FLAGS = OBJECT_END + 0x000F,
    ITEM_FIELD_ENCHANTMENT_1_1 = OBJECT_END + 0x0010,
    ITEM_FIELD_ENCHANTMENT_1_3 = OBJECT_END + 0x0012,
    ITEM_FIELD_ENCHANTMENT_2_1 = OBJECT_END + 0x0013,
    ITEM_FIELD_ENCHANTMENT_2_3 = OBJECT_END + 0x0015,
    ITEM_FIELD_ENCHANTMENT_3_1 = OBJECT_END + 0x0016,
    ITEM_FIELD_ENCHANTMENT_3_3 = OBJECT_END + 0x0018,
    ITEM_FIELD_ENCHANTMENT_4_1 = OBJECT_END + 0x0019,
    ITEM_FIELD_ENCHANTMENT_4_3 = OBJECT_END + 0x001B,
    ITEM_FIELD_ENCHANTMENT_5_1 = OBJECT_END + 0x001C,
    ITEM_FIELD_ENCHANTMENT_5_3 = OBJECT_END + 0x001E,
    ITEM_FIELD_ENCHANTMENT_6_1 = OBJECT_END + 0x001F,
    ITEM_FIELD_ENCHANTMENT_6_3 = OBJECT_END + 0x0021,
    ITEM_FIELD_ENCHANTMENT_7_1 = OBJECT_END + 0x0022,
    ITEM_FIELD_ENCHANTMENT_7_3 = OBJECT_END + 0x0024,
    ITEM_FIELD_ENCHANTMENT_8_1 = OBJECT_END + 0x0025,
    ITEM_FIELD_ENCHANTMENT_8_3 = OBJECT_END + 0x0027,
    ITEM_FIELD_ENCHANTMENT_9_1 = OBJECT_END + 0x0028,
    ITEM_FIELD_ENCHANTMENT_9_3 = OBJECT_END + 0x002A,
    ITEM_FIELD_ENCHANTMENT_10_1 = OBJECT_END + 0x002B,
    ITEM_FIELD_ENCHANTMENT_10_3 = OBJECT_END + 0x002D,
    ITEM_FIELD_ENCHANTMENT_11_1 = OBJECT_END + 0x002E,
    ITEM_FIELD_ENCHANTMENT_11_3 = OBJECT_END + 0x0030,
    ITEM_FIELD_ENCHANTMENT_12_1 = OBJECT_END + 0x0031,
    ITEM_FIELD_ENCHANTMENT_12_3 = OBJECT_END + 0x0033,
    ITEM_FIELD_PROPERTY_SEED = OBJECT_END + 0x0034,
    ITEM_FIELD_RANDOM_PROPERTIES_ID = OBJECT_END + 0x0035,
    ITEM_FIELD_DURABILITY = OBJECT_END + 0x0036,
    ITEM_FIELD_MAXDURABILITY = OBJECT_END + 0x0037,
    ITEM_FIELD_CREATE_PLAYED_TIME = OBJECT_END + 0x0038,
    ITEM_FIELD_PAD = OBJECT_END + 0x0039,
    ITEM_END = OBJECT_END + 0x003A,
};

enum ContainerFields : uint32_t {
    CONTAINER_FIELD_NUM_SLOTS = ITEM_END + 0x0000,
    CONTAINER_ALIGN_PAD = ITEM_END + 0x0001,
    CONTAINER_FIELD_SLOT_1 = ITEM_END + 0x0002,
    CONTAINER_END = ITEM_END + 0x004A,
};

enum UnitFields : uint32_t {
    UNIT_FIELD_CHARM = OBJECT_END + 0x0000,
    UNIT_FIELD_SUMMON = OBJECT_END + 0x0002,
    UNIT_FIELD_CRITTER = OBJECT_END + 0x0004,
    UNIT_FIELD_CHARMEDBY = OBJECT_END + 0x0006,
    UNIT_FIELD_SUMMONEDBY = OBJECT_END + 0x0008,
    UNIT_FIELD_CREATEDBY = OBJECT_END + 0x000A,
    UNIT_FIELD_TARGET = OBJECT_END + 0x000C,
    UNIT_FIELD_CHANNEL_OBJECT = OBJECT_END + 0x000E,
    UNIT_CHANNEL_SPELL = OBJECT_END + 0x0010,
    UNIT_FIELD_BYTES_0 = OBJECT_END + 0x0011,
    UNIT_FIELD_HEALTH = OBJECT_END + 0x0012,
    UNIT_FIELD_POWER1 = OBJECT_END + 0x0013,
    UNIT_FIELD_POWER2 = OBJECT_END + 0x0014,
    UNIT_FIELD_POWER3 = OBJECT_END + 0x0015,
    UNIT_FIELD_POWER4 = OBJECT_END + 0x0016,
    UNIT_FIELD_POWER5 = OBJECT_END + 0x0017,
    UNIT_FIELD_POWER6 = OBJECT_END + 0x0018,
    UNIT_FIELD_POWER7 = OBJECT_END + 0x0019,
    UNIT_FIELD_MAXHEALTH = OBJECT_END + 0x001A,
    UNIT_FIELD_MAXPOWER1 = OBJECT_END + 0x001B,
    UNIT_FIELD_MAXPOWER2 = OBJECT_END + 0x001C,
    UNIT_FIELD_MAXPOWER3 = OBJECT_END + 0x001D,
    UNIT_FIELD_MAXPOWER4 = OBJECT_END + 0x001E,
    UNIT_FIELD_MAXPOWER5 = OBJECT_END + 0x001F,
    UNIT_FIELD_MAXPOWER6 = OBJECT_END + 0x0020,
    UNIT_FIELD_MAXPOWER7 = OBJECT_END + 0x0021,
    UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER = OBJECT_END + 0x0022,
    UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER = OBJECT_END + 0x0029,
    UNIT_FIELD_LEVEL = OBJECT_END + 0x0030,
    UNIT_FIELD_FACTIONTEMPLATE = OBJECT_END + 0x0031,
    UNIT_VIRTUAL_ITEM_SLOT_ID = OBJECT_END + 0x0032,
    UNIT_FIELD_FLAGS = OBJECT_END + 0x0035,
    UNIT_FIELD_FLAGS_2 = OBJECT_END + 0x0036,
    UNIT_FIELD_AURASTATE = OBJECT_END + 0x0037,
    UNIT_FIELD_BASEATTACKTIME = OBJECT_END + 0x0038,
    UNIT_FIELD_RANGEDATTACKTIME = OBJECT_END + 0x003A,
    UNIT_FIELD_BOUNDINGRADIUS = OBJECT_END + 0x003B,
    UNIT_FIELD_COMBATREACH = OBJECT_END + 0x003C,
    UNIT_FIELD_DISPLAYID = OBJECT_END + 0x003D,
    UNIT_FIELD_NATIVEDISPLAYID = OBJECT_END + 0x003E,
    UNIT_FIELD_MOUNTDISPLAYID = OBJECT_END + 0x003F,
    UNIT_FIELD_MINDAMAGE = OBJECT_END + 0x0040,
    UNIT_FIELD_MAXDAMAGE = OBJECT_END + 0x0041,
    UNIT_FIELD_MINOFFHANDDAMAGE = OBJECT_END + 0x0042,
    UNIT_FIELD_MAXOFFHANDDAMAGE = OBJECT_END + 0x0043,
    UNIT_FIELD_BYTES_1 = OBJECT_END + 0x0044,
    UNIT_FIELD_PETNUMBER = OBJECT_END + 0x0045,
    UNIT_FIELD_PET_NAME_TIMESTAMP = OBJECT_END + 0x0046,
    UNIT_FIELD_PETEXPERIENCE = OBJECT_END + 0x0047,
    UNIT_FIELD_PETNEXTLEVELEXP = OBJECT_END + 0x0048,
    UNIT_DYNAMIC_FLAGS = OBJECT_END + 0x0049,
    UNIT_MOD_CAST_SPEED = OBJECT_END + 0x004A,
    UNIT_CREATED_BY_SPELL = OBJECT_END + 0x004B,
    UNIT_NPC_FLAGS = OBJECT_END + 0x004C,
    UNIT_NPC_EMOTESTATE = OBJECT_END + 0x004D,
    UNIT_FIELD_STAT0 = OBJECT_END + 0x004E,
    UNIT_FIELD_STAT1 = OBJECT_END + 0x004F,
    UNIT_FIELD_STAT2 = OBJECT_END + 0x0050,
    UNIT_FIELD_STAT3 = OBJECT_END + 0x0051,
    UNIT_FIELD_STAT4 = OBJECT_END + 0x0052,
    UNIT_FIELD_POSSTAT0 = OBJECT_END + 0x0053,
    UNIT_FIELD_POSSTAT1 = OBJECT_END + 0x0054,
    UNIT_FIELD_POSSTAT2 = OBJECT_END + 0x0055,
    UNIT_FIELD_POSSTAT3 = OBJECT_END + 0x0056,
    UNIT_FIELD_POSSTAT4 = OBJECT_END + 0x0057,
    UNIT_FIELD_NEGSTAT0 = OBJECT_END + 0x0058,
    UNIT_FIELD_NEGSTAT1 = OBJECT_END + 0x0059,
    UNIT_FIELD_NEGSTAT2 = OBJECT_END + 0x005A,
    UNIT_FIELD_NEGSTAT3 = OBJECT_END + 0x005B,
    UNIT_FIELD_NEGSTAT4 = OBJECT_END + 0x005C,
    UNIT_FIELD_RESISTANCES = OBJECT_END + 0x005D,
    UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE = OBJECT_END + 0x0064,
    UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE = OBJECT_END + 0x006B,
    UNIT_FIELD_BASE_MANA = OBJECT_END + 0x0072,
    UNIT_FIELD_BASE_HEALTH = OBJECT_END + 0x0073,
    UNIT_FIELD_BYTES_2 = OBJECT_END + 0x0074,
    UNIT_FIELD_ATTACK_POWER = OBJECT_END + 0x0075,
    UNIT_FIELD_ATTACK_POWER_MODS = OBJECT_END + 0x0076,
    UNIT_FIELD_ATTACK_POWER_MULTIPLIER = OBJECT_END + 0x0077,
    UNIT_FIELD_RANGED_ATTACK_POWER = OBJECT_END + 0x0078,
    UNIT_FIELD_RANGED_ATTACK_POWER_MODS = OBJECT_END + 0x0079,
    UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER = OBJECT_END + 0x007A,
    UNIT_FIELD_MINRANGEDDAMAGE = OBJECT_END + 0x007B,
    UNIT_FIELD_MAXRANGEDDAMAGE = OBJECT_END + 0x007C,
    UNIT_FIELD_POWER_COST_MODIFIER = OBJECT_END + 0x007D,
    UNIT_FIELD_POWER_COST_MULTIPLIER = OBJECT_END + 0x0084,
    UNIT_FIELD_MAXHEALTHMODIFIER = OBJECT_END + 0x008B,
    UNIT_FIELD_HOVERHEIGHT = OBJECT_END + 0x008C,
    UNIT_FIELD_PADDING = OBJECT_END + 0x008D,
    UNIT_END = OBJECT_END + 0x008E,
};

enum PlayerFields : uint32_t {
    PLAYER_DUEL_ARBITER = UNIT_END + 0x0000,
    PLAYER_FLAGS = UNIT_END + 0x0002,
    PLAYER_GUILDID = UNIT_END + 0x0003,
    PLAYER_GUILDRANK = UNIT_END + 0x0004,
    PLAYER_BYTES = UNIT_END + 0x0005,
    PLAYER_BYTES_2 = UNIT_END + 0x0006,
    PLAYER_BYTES_3 = UNIT_END + 0x0007,
    PLAYER_DUEL_TEAM = UNIT_END + 0x0008,
    PLAYER_GUILD_TIMESTAMP = UNIT_END + 0x0009,
    PLAYER_QUEST_LOG_1_1 = UNIT_END + 0x000A,
    PLAYER_QUEST_LOG_1_2 = UNIT_END + 0x000B,
    PLAYER_QUEST_LOG_1_3 = UNIT_END + 0x000C,
    PLAYER_QUEST_LOG_1_4 = UNIT_END + 0x000E,
    PLAYER_QUEST_LOG_2_1 = UNIT_END + 0x000F,
    PLAYER_QUEST_LOG_2_2 = UNIT_END + 0x0010,
    PLAYER_QUEST_LOG_2_3 = UNIT_END + 0x0011,
    PLAYER_QUEST_LOG_2_5 = UNIT_END + 0x0013,
    PLAYER_QUEST_LOG_3_1 = UNIT_END + 0x0014,
    PLAYER_QUEST_LOG_3_2 = UNIT_END + 0x0015,
    PLAYER_QUEST_LOG_3_3 = UNIT_END + 0x0016,
    PLAYER_QUEST_LOG_3_5 = UNIT_END + 0x0018,
    PLAYER_QUEST_LOG_4_1 = UNIT_END + 0x0019,
    PLAYER_QUEST_LOG_4_2 = UNIT_END + 0x001A,
    PLAYER_QUEST_LOG_4_3 = UNIT_END + 0x001B,
    PLAYER_QUEST_LOG_4_5 = UNIT_END + 0x001D,
    PLAYER_QUEST_LOG_5_1 = UNIT_END + 0x001E,
    PLAYER_QUEST_LOG_5_2 = UNIT_END + 0x001F,
    PLAYER_QUEST_LOG_5_3 = UNIT_END + 0x0020,
    PLAYER_QUEST_LOG_5_5 = UNIT_END + 0x0022,
    PLAYER_QUEST_LOG_6_1 = UNIT_END + 0x0023,
    PLAYER_QUEST_LOG_6_2 = UNIT_END + 0x0024,
    PLAYER_QUEST_LOG_6_3 = UNIT_END + 0x0025,
    PLAYER_QUEST_LOG_6_5 = UNIT_END + 0x0027,
    PLAYER_QUEST_LOG_7_1 = UNIT_END + 0x0028,
    PLAYER_QUEST_LOG_7_2 = UNIT_END + 0x0029,
    PLAYER_QUEST_LOG_7_3 = UNIT_END + 0x002A,
    PLAYER_QUEST_LOG_7_5 = UNIT_END + 0x002C,
    PLAYER_QUEST_LOG_8_1 = UNIT_END + 0x002D,
    PLAYER_QUEST_LOG_8_2 = UNIT_END + 0x002E,
    PLAYER_QUEST_LOG_8_3 = UNIT_END + 0x002F,
    PLAYER_QUEST_LOG_8_5 = UNIT_END + 0x0031,
    PLAYER_QUEST_LOG_9_1 = UNIT_END + 0x0032,
    PLAYER_QUEST_LOG_9_2 = UNIT_END + 0x0033,
    PLAYER_QUEST_LOG_9_3 = UNIT_END + 0x0034,
    PLAYER_QUEST_LOG_9_5 = UNIT_END + 0x0036,
    PLAYER_QUEST_LOG_10_1 = UNIT_END + 0x0037,
    PLAYER_QUEST_LOG_10_2 = UNIT_END + 0x0038,
    PLAYER_QUEST_LOG_10_3 = UNIT_END + 0x0039,
    PLAYER_QUEST_LOG_10_5 = UNIT_END + 0x003B,
    PLAYER_QUEST_LOG_11_1 = UNIT_END + 0x003C,
    PLAYER_QUEST_LOG_11_2 = UNIT_END + 0x003D,
    PLAYER_QUEST_LOG_11_3 = UNIT_END + 0x003E,
    PLAYER_QUEST_LOG_11_5 = UNIT_END + 0x0040,
    PLAYER_QUEST_LOG_12_1 = UNIT_END + 0x0041,
    PLAYER_QUEST_LOG_12_2 = UNIT_END + 0x0042,
    PLAYER_QUEST_LOG_12_3 = UNIT_END + 0x0043,
    PLAYER_QUEST_LOG_12_5 = UNIT_END + 0x0045,
    PLAYER_QUEST_LOG_13_1 = UNIT_END + 0x0046,
    PLAYER_QUEST_LOG_13_2 = UNIT_END + 0x0047,
    PLAYER_QUEST_LOG_13_3 = UNIT_END + 0x0048,
    PLAYER_QUEST_LOG_13_5 = UNIT_END + 0x004A,
    PLAYER_QUEST_LOG_14_1 = UNIT_END + 0x004B,
    PLAYER_QUEST_LOG_14_2 = UNIT_END + 0x004C,
    PLAYER_QUEST_LOG_14_3 = UNIT_END + 0x004D,
    PLAYER_QUEST_LOG_14_5 = UNIT_END + 0x004F,
    PLAYER_QUEST_LOG_15_1 = UNIT_END + 0x0050,
    PLAYER_QUEST_LOG_15_2 = UNIT_END + 0x0051,
    PLAYER_QUEST_LOG_15_3 = UNIT_END + 0x0052,
    PLAYER_QUEST_LOG_15_5 = UNIT_END + 0x0054,
    PLAYER_QUEST_LOG_16_1 = UNIT_END + 0x0055,
    PLAYER_QUEST_LOG_16_2 = UNIT_END + 0x0056,
    PLAYER_QUEST_LOG_16_3 = UNIT_END + 0x0057,
    PLAYER_QUEST_LOG_16_5 = UNIT_END + 0x0059,
    PLAYER_QUEST_LOG_17_1 = UNIT_END + 0x005A,
    PLAYER_QUEST_LOG_17_2 = UNIT_END + 0x005B,
    PLAYER_QUEST_LOG_17_3 = UNIT_END + 0x005C,
    PLAYER_QUEST_LOG_17_5 = UNIT_END + 0x005E,
    PLAYER_QUEST_LOG_18_1 = UNIT_END + 0x005F,
    PLAYER_QUEST_LOG_18_2 = UNIT_END + 0x0060,
    PLAYER_QUEST_LOG_18_3 = UNIT_END + 0x0061,
    PLAYER_QUEST_LOG_18_5 = UNIT_END + 0x0063,
    PLAYER_QUEST_LOG_19_1 = UNIT_END + 0x0064,
    PLAYER_QUEST_LOG_19_2 = UNIT_END + 0x0065,
    PLAYER_QUEST_LOG_19_3 = UNIT_END + 0x0066,
    PLAYER_QUEST_LOG_19_5 = UNIT_END + 0x0068,
    PLAYER_QUEST_LOG_20_1 = UNIT_END + 0x0069,
    PLAYER_QUEST_LOG_20_2 = UNIT_END + 0x006A,
    PLAYER_QUEST_LOG_20_3 = UNIT_END + 0x006B,
    PLAYER_QUEST_LOG_20_5 = UNIT_END + 0x006D,
    PLAYER_QUEST_LOG_21_1 = UNIT_END + 0x006E,
    PLAYER_QUEST_LOG_21_2 = UNIT_END + 0x006F,
    PLAYER_QUEST_LOG_21_3 = UNIT_END + 0x0070,
    PLAYER_QUEST_LOG_21_5 = UNIT_END + 0x0072,
    PLAYER_QUEST_LOG_22_1 = UNIT_END + 0x0073,
    PLAYER_QUEST_LOG_22_2 = UNIT_END + 0x0074,
    PLAYER_QUEST_LOG_22_3 = UNIT_END + 0x0075,
    PLAYER_QUEST_LOG_22_5 = UNIT_END + 0x0077,
    PLAYER_QUEST_LOG_23_1 = UNIT_END + 0x0078,
    PLAYER_QUEST_LOG_23_2 = UNIT_END + 0x0079,
    PLAYER_QUEST_LOG_23_3 = UNIT_END + 0x007A,
    PLAYER_QUEST_LOG_23_5 = UNIT_END + 0x007C,
    PLAYER_QUEST_LOG_24_1 = UNIT_END + 0x007D,
    PLAYER_QUEST_LOG_24_2 = UNIT_END + 0x007E,
    PLAYER_QUEST_LOG_24_3 = UNIT_END + 0x007F,
    PLAYER_QUEST_LOG_24_5 = UNIT_END + 0x0081,
    PLAYER_QUEST_LOG_25_1 = UNIT_END + 0x0082,
    PLAYER_QUEST_LOG_25_2 = UNIT_END + 0x0083,
    PLAYER_QUEST_LOG_25_3 = UNIT_END + 0x0084,
    PLAYER_QUEST_LOG_25_5 = UNIT_END + 0x0086,
    PLAYER_VISIBLE_ITEM_1_ENTRYID = UNIT_END + 0x0087,
    PLAYER_VISIBLE_ITEM_1_ENCHANTMENT = UNIT_END + 0x0088,
    PLAYER_VISIBLE_ITEM_2_ENTRYID = UNIT_END + 0x0089,
    PLAYER_VISIBLE_ITEM_2_ENCHANTMENT = UNIT_END + 0x008A,
    PLAYER_VISIBLE_ITEM_3_ENTRYID = UNIT_END + 0x008B,
    PLAYER_VISIBLE_ITEM_3_ENCHANTMENT = UNIT_END + 0x008C,
    PLAYER_VISIBLE_ITEM_4_ENTRYID = UNIT_END + 0x008D,
    PLAYER_VISIBLE_ITEM_4_ENCHANTMENT = UNIT_END + 0x008E,
    PLAYER_VISIBLE_ITEM_5_ENTRYID = UNIT_END + 0x008F,
    PLAYER_VISIBLE_ITEM_5_ENCHANTMENT = UNIT_END + 0x0090,
    PLAYER_VISIBLE_ITEM_6_ENTRYID = UNIT_END + 0x0091,
    PLAYER_VISIBLE_ITEM_6_ENCHANTMENT = UNIT_END + 0x0092,
    PLAYER_VISIBLE_ITEM_7_ENTRYID = UNIT_END + 0x0093,
    PLAYER_VISIBLE_ITEM_7_ENCHANTMENT = UNIT_END + 0x0094,
    PLAYER_VISIBLE_ITEM_8_ENTRYID = UNIT_END + 0x0095,
    PLAYER_VISIBLE_ITEM_8_ENCHANTMENT = UNIT_END + 0x0096,
    PLAYER_VISIBLE_ITEM_9_ENTRYID = UNIT_END + 0x0097,
    PLAYER_VISIBLE_ITEM_9_ENCHANTMENT = UNIT_END + 0x0098,
    PLAYER_VISIBLE_ITEM_10_ENTRYID = UNIT_END + 0x0099,
    PLAYER_VISIBLE_ITEM_10_ENCHANTMENT = UNIT_END + 0x009A,
    PLAYER_VISIBLE_ITEM_11_ENTRYID = UNIT_END + 0x009B,
    PLAYER_VISIBLE_ITEM_11_ENCHANTMENT = UNIT_END + 0x009C,
    PLAYER_VISIBLE_ITEM_12_ENTRYID = UNIT_END + 0x009D,
    PLAYER_VISIBLE_ITEM_12_ENCHANTMENT = UNIT_END + 0x009E,
    PLAYER_VISIBLE_ITEM_13_ENTRYID = UNIT_END + 0x009F,
    PLAYER_VISIBLE_ITEM_13_ENCHANTMENT = UNIT_END + 0x00A0,
    PLAYER_VISIBLE_ITEM_14_ENTRYID = UNIT_END + 0x00A1,
    PLAYER_VISIBLE_ITEM_14_ENCHANTMENT = UNIT_END + 0x00A2,
    PLAYER_VISIBLE_ITEM_15_ENTRYID = UNIT_END + 0x00A3,
    PLAYER_VISIBLE_ITEM_15_ENCHANTMENT = UNIT_END + 0x00A4,
    PLAYER_VISIBLE_ITEM_16_ENTRYID = UNIT_END + 0x00A5,
    PLAYER_VISIBLE_ITEM_16_ENCHANTMENT = UNIT_END + 0x00A6,
    PLAYER_VISIBLE_ITEM_17_ENTRYID = UNIT_END + 0x00A7,
    PLAYER_VISIBLE_ITEM_17_ENCHANTMENT = UNIT_END + 0x00A8,
    PLAYER_VISIBLE_ITEM_18_ENTRYID = UNIT_END + 0x00A9,
    PLAYER_VISIBLE_ITEM_18_ENCHANTMENT = UNIT_END + 0x00AA,
    PLAYER_VISIBLE_ITEM_19_ENTRYID = UNIT_END + 0x00AB,
    PLAYER_VISIBLE_ITEM_19_ENCHANTMENT = UNIT_END + 0x00AC,
    PLAYER_CHOSEN_TITLE = UNIT_END + 0x00AD,
    PLAYER_FAKE_INEBRIATION = UNIT_END + 0x00AE,
    PLAYER_FIELD_PAD_0 = UNIT_END + 0x00AF,
    PLAYER_FIELD_INV_SLOT_HEAD = UNIT_END + 0x00B0,
    PLAYER_FIELD_PACK_SLOT_1 = UNIT_END + 0x00DE,
    PLAYER_FIELD_BANK_SLOT_1 = UNIT_END + 0x00FE,
    PLAYER_FIELD_BANKBAG_SLOT_1 = UNIT_END + 0x0136,
    PLAYER_FIELD_VENDORBUYBACK_SLOT_1 = UNIT_END + 0x0144,
    PLAYER_FIELD_KEYRING_SLOT_1 = UNIT_END + 0x015C,
    PLAYER_FIELD_CURRENCYTOKEN_SLOT_1 = UNIT_END + 0x019C,
    PLAYER_FARSIGHT = UNIT_END + 0x01DC,
    PLAYER__FIELD_KNOWN_TITLES = UNIT_END + 0x01DE,
    PLAYER__FIELD_KNOWN_TITLES1 = UNIT_END + 0x01E0,
    PLAYER__FIELD_KNOWN_TITLES2 = UNIT_END + 0x01E2,
    PLAYER_FIELD_KNOWN_CURRENCIES = UNIT_END + 0x01E4,
    PLAYER_XP = UNIT_END + 0x01E6,
    PLAYER_NEXT_LEVEL_XP = UNIT_END + 0x01E7,
    PLAYER_SKILL_INFO_1_1 = UNIT_END + 0x01E8,
    PLAYER_CHARACTER_POINTS1 = UNIT_END + 0x0368,
    PLAYER_CHARACTER_POINTS2 = UNIT_END + 0x0369,
    PLAYER_TRACK_CREATURES = UNIT_END + 0x036A,
    PLAYER_TRACK_RESOURCES = UNIT_END + 0x036B,
    PLAYER_BLOCK_PERCENTAGE = UNIT_END + 0x036C,
    PLAYER_DODGE_PERCENTAGE = UNIT_END + 0x036D,
    PLAYER_PARRY_PERCENTAGE = UNIT_END + 0x036E,
    PLAYER_EXPERTISE = UNIT_END + 0x036F,
    PLAYER_OFFHAND_EXPERTISE = UNIT_END + 0x0370,
    PLAYER_CRIT_PERCENTAGE = UNIT_END + 0x0371,
    PLAYER_RANGED_CRIT_PERCENTAGE = UNIT_END + 0x0372,
    PLAYER_OFFHAND_CRIT_PERCENTAGE = UNIT_END + 0x0373,
    PLAYER_SPELL_CRIT_PERCENTAGE1 = UNIT_END + 0x0374,
    PLAYER_SHIELD_BLOCK = UNIT_END + 0x037B,
    PLAYER_SHIELD_BLOCK_CRIT_PERCENTAGE = UNIT_END + 0x037C,
    PLAYER_EXPLORED_ZONES_1 = UNIT_END + 0x037D,
    PLAYER_REST_STATE_EXPERIENCE = UNIT_END + 0x03FD,
    PLAYER_FIELD_COINAGE = UNIT_END + 0x03FE,
    PLAYER_FIELD_MOD_DAMAGE_DONE_POS = UNIT_END + 0x03FF,
    PLAYER_FIELD_MOD_DAMAGE_DONE_NEG = UNIT_END + 0x0406,
    PLAYER_FIELD_MOD_DAMAGE_DONE_PCT = UNIT_END + 0x040D,
    PLAYER_FIELD_MOD_HEALING_DONE_POS = UNIT_END + 0x0414,
    PLAYER_FIELD_MOD_HEALING_PCT = UNIT_END + 0x0415,
    PLAYER_FIELD_MOD_HEALING_DONE_PCT = UNIT_END + 0x0416,
    PLAYER_FIELD_MOD_TARGET_RESISTANCE = UNIT_END + 0x0417,
    PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE = UNIT_END + 0x0418,
    PLAYER_FIELD_BYTES = UNIT_END + 0x0419,
    PLAYER_AMMO_ID = UNIT_END + 0x041A,
    PLAYER_SELF_RES_SPELL = UNIT_END + 0x041B,
    PLAYER_FIELD_PVP_MEDALS = UNIT_END + 0x041C,
    PLAYER_FIELD_BUYBACK_PRICE_1 = UNIT_END + 0x041D,
    PLAYER_FIELD_BUYBACK_TIMESTAMP_1 = UNIT_END + 0x0429,
    PLAYER_FIELD_KILLS = UNIT_END + 0x0435,
    PLAYER_FIELD_TODAY_CONTRIBUTION = UNIT_END + 0x0436,
    PLAYER_FIELD_YESTERDAY_CONTRIBUTION = UNIT_END + 0x0437,
    PLAYER_FIELD_LIFETIME_HONORBALE_KILLS = UNIT_END + 0x0438,
    PLAYER_FIELD_BYTES2 = UNIT_END + 0x0439,
    PLAYER_FIELD_WATCHED_FACTION_INDEX = UNIT_END + 0x043A,
    PLAYER_FIELD_COMBAT_RATING_1 = UNIT_END + 0x043B,
    PLAYER_FIELD_ARENA_TEAM_INFO_1_1 = UNIT_END + 0x0454,
    PLAYER_FIELD_HONOR_CURRENCY = UNIT_END + 0x0469,
    PLAYER_FIELD_ARENA_CURRENCY = UNIT_END + 0x046A,
    PLAYER_FIELD_MAX_LEVEL = UNIT_END + 0x046B,
    PLAYER_FIELD_DAILY_QUESTS_1 = UNIT_END + 0x046C,
    PLAYER_RUNE_REGEN_1 = UNIT_END + 0x0485,
    PLAYER_NO_REAGENT_COST_1 = UNIT_END + 0x0489,
    PLAYER_FIELD_GLYPH_SLOTS_1 = UNIT_END + 0x048C,
    PLAYER_FIELD_GLYPHS_1 = UNIT_END + 0x0492,
    PLAYER_GLYPHS_ENABLED = UNIT_END + 0x0498,
    PLAYER_PET_SPELL_POWER = UNIT_END + 0x0499,
    PLAYER_END = UNIT_END + 0x049A,
};

enum GameObjectFields : uint32_t {
    OBJECT_FIELD_CREATED_BY = OBJECT_END + 0x0000,
    GAMEOBJECT_DISPLAYID = OBJECT_END + 0x0002,
    GAMEOBJECT_FLAGS = OBJECT_END + 0x0003,
    GAMEOBJECT_PARENTROTATION = OBJECT_END + 0x0004,
    GAMEOBJECT_DYNAMIC = OBJECT_END + 0x0008,
    GAMEOBJECT_FACTION = OBJECT_END + 0x0009,
    GAMEOBJECT_LEVEL = OBJECT_END + 0x000A,
    GAMEOBJECT_BYTES_1 = OBJECT_END + 0x000B,
    GAMEOBJECT_END = OBJECT_END + 0x000C,
};

enum DynamicObjectFields : uint32_t {
    DYNAMICOBJECT_CASTER = OBJECT_END + 0x0000,
    DYNAMICOBJECT_BYTES = OBJECT_END + 0x0002,
    DYNAMICOBJECT_SPELLID = OBJECT_END + 0x0003,
    DYNAMICOBJECT_RADIUS = OBJECT_END + 0x0004,
    DYNAMICOBJECT_CASTTIME = OBJECT_END + 0x0005,
    DYNAMICOBJECT_END = OBJECT_END + 0x0006,
};

enum CorpseFields : uint32_t {
    CORPSE_FIELD_OWNER = OBJECT_END + 0x0000,
    CORPSE_FIELD_PARTY = OBJECT_END + 0x0002,
    CORPSE_FIELD_DISPLAY_ID = OBJECT_END + 0x0004,
    CORPSE_FIELD_ITEM = OBJECT_END + 0x0005,
    CORPSE_FIELD_BYTES_1 = OBJECT_END + 0x0018,
    CORPSE_FIELD_BYTES_2 = OBJECT_END + 0x0019,
    CORPSE_FIELD_GUILD = OBJECT_END + 0x001A,
    CORPSE_FIELD_FLAGS = OBJECT_END + 0x001B,
    CORPSE_FIELD_DYNAMIC_FLAGS = OBJECT_END + 0x001C,
    CORPSE_FIELD_PAD = OBJECT_END + 0x001D,
    CORPSE_END = OBJECT_END + 0x001E,
};

} // namespace WoWDescriptors

namespace WoWOffsets {

// Organised structured offsets matching IceFlake pointers
namespace DirectX {
    constexpr uint32_t DirectXBase = 0xC5DF88;
    constexpr uint32_t Device = 0x397C;
    constexpr uint32_t EndScene = 0xA8;
}

namespace ObjectManager {
    constexpr uint32_t EnumVisibleObjects = 0x004D4B30;
    constexpr uint32_t GetObjectByGuid = 0x004D4DB0;
    constexpr uint32_t GetLocalPlayerGuid = 0x004D3790;
}

namespace Object {
    constexpr uint32_t GetObjectName = 54;
    constexpr uint32_t GetObjectLocation = 12;
    constexpr uint32_t GetObjectFacing = 14;
    constexpr uint32_t Interact = 44;
    constexpr uint32_t SelectObject = 0x00524BF0;
}

namespace Item {
    constexpr uint32_t UseItem = 0x00708C20;
    constexpr uint32_t CanUseItem = 0x006DC3F0;
}

namespace Container {
    constexpr uint32_t GetBagAtIndex = 0x005D6F20;
    constexpr uint32_t LootWindowOffset = 0x00BFA8D8;
}

namespace Unit {
    constexpr uint32_t ChanneledCastingId = 0xA80;
    constexpr uint32_t CastingId = 0xA6C;
    constexpr uint32_t UpdateDisplayInfo = 0x73e410;
    constexpr uint32_t UnitReaction = 0x007251C0;
    constexpr uint32_t HasAuraBySpellId = 0x007282A0;
    constexpr uint32_t GetAura = 0x00556E10;
    constexpr uint32_t GetAuraCount = 0x004F8850;
    constexpr uint32_t GetCreatureType = 0x0071F300;
    constexpr uint32_t GetCreatureRank = 0x00718A00;
    constexpr uint32_t ShapeshiftFormId = 0x0071AF70;
    constexpr uint32_t CalculateThreat = 0x007374C0;
}

namespace LocalPlayer {
    constexpr uint32_t ClickToMove = 0x00727400;
    constexpr uint32_t SetFacing = 0x0072EA50;
    constexpr uint32_t IsClickMoving = 0x00721F90;
    constexpr uint32_t StopCTM = 0x0072B3A0;
    constexpr uint32_t CorpsePosition = 0x0051F430;
    constexpr uint32_t ComboPoints = 0x00BD084D;
    constexpr uint32_t ComboPointsTarget = 0x00BD08A8;
    constexpr uint32_t CompletedQuests = 0x00ACFDF4;
    constexpr uint32_t RuneState = 0xC24388;
    constexpr uint32_t RuneType = 0xC24304;
    constexpr uint32_t RuneCooldown = 0xC24364;
}

namespace Spell {
    constexpr uint32_t SpellCount = 0x00BE8D9C;
    constexpr uint32_t SpellBook = 0x00BE5D88;
    constexpr uint32_t CastSpell = 0x0080DA40;
    constexpr uint32_t GetSpellCooldown = 0x00809000;
    constexpr uint32_t FirstActionBarSpellId = 0x00C1E358;
}

namespace World {
    constexpr uint32_t Traceline = 0x007A3B70;
    constexpr uint32_t HandleTerrainClick = 0x00527830;
    constexpr uint32_t CurrentMapId = 0x00AB63BC;
    constexpr uint32_t InternalMapName = 0x00CE06D0;
    constexpr uint32_t ZoneID = 0x00BD080C;
    constexpr uint32_t ZoneText = 0x00BD0788;
    constexpr uint32_t SubZoneText = 0x00BD0784;
}

namespace LuaInterface {
    constexpr uint32_t LuaState = 0x00D3F78C;
    constexpr uint32_t LuaLoadBuffer = 0x0084F860;
    constexpr uint32_t LuaPCall = 0x0084EC50;
    constexpr uint32_t LuaGetTop = 0x0084DBD0;
    constexpr uint32_t LuaSetTop = 0x0084DBF0;
    constexpr uint32_t LuaType = 0x0084DEB0;
    constexpr uint32_t LuaToNumber = 0x0084E030;
    constexpr uint32_t LuaToLString = 0x0084E0E0;
    constexpr uint32_t LuaToBoolean = 0x0084E0B0;
}

namespace Events {
    constexpr uint32_t EventVictim = 0x00511C40;
}

namespace DBC {
    constexpr uint32_t RegisterBase = 0x006337D0;
    constexpr uint32_t GetRow = 0x004BB1C0;
    constexpr uint32_t GetLocalizedRow = 0x004CFD20;
}

namespace WDB {
    constexpr uint32_t DbWoWCache_GetInfoBlockById = 0x0067FA80;
    constexpr uint32_t DdItemCache_GetInfoBlockByID = 0x0067CA30;
    constexpr uint32_t DbQuestCache_GetInfoBlockByID = 0x0067DE90;
    constexpr uint32_t WdbItemCache = 0x00C5D828;
    constexpr uint32_t WdbQuestCache = 0x00C5DA48;
}

namespace Drawing {
    constexpr uint32_t WorldFrame = 0x00B7436C;
    constexpr uint32_t ActiveCamera = 0x7E20;
    constexpr uint32_t RenderBackground = 0x2532E0;
}

namespace Other {
    constexpr uint32_t PerformanceCounter = 0x0086AE20;
    constexpr uint32_t LastHardwareAction = 0x00B499A4;
    constexpr uint32_t IsBobbing = 0xBC;
    constexpr uint32_t WorldLoading = 0x00B6AA38;
    constexpr uint32_t WorldLoaded = 0x00BEBA40;
    constexpr uint32_t GameState = 0x00B6A9E0;
    constexpr uint32_t RealmName = 0x00C79B9E;
    constexpr uint32_t AHListAuctions = 0xc0f448;
    constexpr uint32_t AHListNumAuctions = 0xc0f444;
    constexpr uint32_t AHListTotalAuctions = 0xc0f408;
}

namespace Party {
    constexpr uint32_t PartyArray = 0x00BD1948;
    constexpr uint32_t DungeonDifficulty = 0x00BD0898;
}

namespace Raid {
    constexpr uint32_t RaidCount = 0x00BEB608;
    constexpr uint32_t RaidArray = 0x00BEB568;
    constexpr uint32_t RaidDifficulty = 0x00BD089C;
}

namespace Console {
    constexpr uint32_t Enable = 0x00CABCC4;
    constexpr uint32_t WriteA = 0x00765360;
    constexpr uint32_t RegisterCommand = 0x00769100;
    constexpr uint32_t UnregisterCommand = 0x007689E0;
    constexpr uint32_t InvalidPtrCheck = 0x00D415B8;
}

namespace Packets {
    constexpr uint32_t Initialize = 0x00401050;
    constexpr uint32_t PutInt8 = 0x0047AFE0;
    constexpr uint32_t PutInt16 = 0x0047B040;
    constexpr uint32_t PutInt32 = 0x0047B0A0;
    constexpr uint32_t PutInt64 = 0x0047B100;
    constexpr uint32_t PutFloat = 0x0047B160;
    constexpr uint32_t PutString = 0x0047B300;
    constexpr uint32_t PutBytes = 0x0047B1C0;
    constexpr uint32_t GetInt8 = 0x0047B340;
    constexpr uint32_t GetInt16 = 0x0047B380;
    constexpr uint32_t GetInt32 = 0x0047B3C0;
    constexpr uint32_t GetInt64 = 0x0047B400;
    constexpr uint32_t GetFloat = 0x0047B440;
    constexpr uint32_t GetString = 0x0047B480;
    constexpr uint32_t GetBytes = 0x0047B560;
    constexpr uint32_t Finalize = 0x00401130;
    constexpr uint32_t Destroy = 0x00403880;
}

namespace ClientServices {
    constexpr uint32_t SendPacket = 0x00406F40;
    constexpr uint32_t SendPacket2 = 0x00632B50;
    constexpr uint32_t GetCurrent = 0x006B0970;
    constexpr uint32_t SetMessageHandler = 0x006B0B80;
}


// ---------------------------------------------------------------------------
// Flat Legacy Aliases for backward compatibility
// ---------------------------------------------------------------------------
constexpr uint32_t arenaPlayer1        = 0xBE9F48;
constexpr uint32_t arenaPlayer2        = arenaPlayer1 + 0x8;
constexpr uint32_t arenaPlayer3        = arenaPlayer2 + 0x8;
constexpr uint32_t arenaPlayer4        = arenaPlayer3 + 0x8;
constexpr uint32_t arenaPlayer5        = arenaPlayer4 + 0x8;
constexpr uint32_t battlegroundStatus  = 0xBEA4D0;
constexpr uint32_t isBattlegroundOver  = 0xBEA588;

constexpr uint32_t characterSlotSelected = 0x6C436C;

constexpr uint32_t clientGameUITarget   = Object::SelectObject;
constexpr uint32_t clientObjectManagerGetActivePlayerObject = 0x4038F0;

constexpr uint32_t continentName        = World::InternalMapName;
constexpr uint32_t mapID                = World::CurrentMapId;
constexpr uint32_t zoneID               = World::ZoneID;
constexpr uint32_t zoneText             = World::ZoneText;
constexpr uint32_t subZoneText          = World::SubZoneText;
constexpr uint32_t zoneNamePointer      = 0xBD0780;

constexpr uint32_t corpseX              = 0xBD0A58;   // float
constexpr uint32_t corpseY              = corpseX + 0x4;
constexpr uint32_t corpseZ              = corpseY + 0x4;
constexpr uint32_t playerCorpseX        = 0xBD0A58;
constexpr uint32_t playerCorpseY        = playerCorpseX + 0x4;
constexpr uint32_t playerCorpseZ        = playerCorpseY + 0x4;

constexpr uint32_t ctmABase             = 0xCA11D8;
constexpr uint32_t ctmAction            = ctmABase + 0x1C;
constexpr uint32_t ctmDistance          = ctmABase + 0xC;
constexpr uint32_t ctmGUID              = ctmABase + 0x20;
constexpr uint32_t ctmX                 = ctmABase + 0x8C;
constexpr uint32_t ctmY                 = ctmABase + 0x90;
constexpr uint32_t ctmZ                 = ctmABase + 0x94;

constexpr uint32_t currentClientConnection = 0xC79CE0;
constexpr uint32_t currentManagerLocalGUID = 0xC0;
constexpr uint32_t currentManagerOffset    = 0x2ED0;

constexpr uint32_t devicePtr1           = DirectX::DirectXBase;
constexpr uint32_t devicePtr2           = DirectX::Device;
constexpr uint32_t endScene             = DirectX::EndScene;

constexpr uint32_t dynamicObjectBytes   = WoWDescriptors::DYNAMICOBJECT_BYTES;
constexpr uint32_t dynamicObjectCaster  = WoWDescriptors::DYNAMICOBJECT_CASTER;
constexpr uint32_t dynamicObjectCastTime= WoWDescriptors::DYNAMICOBJECT_CASTTIME;
constexpr uint32_t dynamicObjectRadius  = WoWDescriptors::DYNAMICOBJECT_RADIUS;
constexpr uint32_t dynamicObjectSpellID = WoWDescriptors::DYNAMICOBJECT_SPELLID;

constexpr uint32_t firstObjectOffset    = 0xAC;
constexpr uint32_t nextObjectOffset     = 0x3C;

constexpr uint32_t gameobjectGUIDOffset = 0x30;
constexpr uint32_t gameobjectTypeOffset = 0x14;

constexpr uint32_t gameState            = Other::GameState;
constexpr uint32_t isLoading            = Other::WorldLoading;
constexpr uint32_t isIndoor             = 0xB4AA94;
constexpr uint32_t worldLoaded          = Other::WorldLoaded;

constexpr uint32_t localComboPoint         = 0xBD0845;
constexpr uint32_t localLastTarget         = 0xBD07B0;
constexpr uint32_t localLootWindowOpen     = Container::LootWindowOffset;
constexpr uint32_t localMouseoverGUID      = 0xBD07B0;
constexpr uint32_t localPlayerCharacterState        = 0x6DACA4;
constexpr uint32_t localPlayerCharacterStateOffset1 = 0xC;
constexpr uint32_t localPlayerCharacterStateOffset2 = 0x94;
constexpr uint32_t localPlayerCharacterStateOffset3 = 0x90;
constexpr uint32_t localPlayerGUID         = 0xCA1238;
constexpr uint32_t localTargetGUID         = 0xBD07B0;
constexpr uint32_t playerBase              = 0xCD87A8;
constexpr uint32_t playerHealth            = 0x19B8;
constexpr uint32_t playerIsIngame          = 0xBD0792;
constexpr uint32_t playerIsLoadingscreen   = Other::WorldLoading;
constexpr uint32_t playerName              = Other::RealmName - 0x86; // 0xC79D18

constexpr uint32_t luaDoString             = 0x819210;
constexpr uint32_t luaGetLocalizedText     = 0x7225E0;
constexpr uint32_t luaState                = LuaInterface::LuaState;
constexpr int32_t  luaGlobalsIndex         = -10002;

constexpr uint32_t luaGetTop               = LuaInterface::LuaGetTop;
constexpr uint32_t luaSetTop               = LuaInterface::LuaSetTop;
constexpr uint32_t luaPushString           = 0x0084E350;
constexpr uint32_t luaPushInteger          = 0x0084E2D0;
constexpr uint32_t luaPushNumber           = 0x0084E2A0;
constexpr uint32_t luaPushBoolean          = 0x0084E4D0;
constexpr uint32_t luaPushCClosure         = 0x0084E400;
constexpr uint32_t luaToLString            = LuaInterface::LuaToLString;
constexpr uint32_t luaToNumber             = LuaInterface::LuaToNumber;
constexpr uint32_t luaToInteger            = 0x0084E070;
constexpr uint32_t luaToBoolean            = 0x0044E2C0;
constexpr uint32_t luaToCFunction          = 0x0084E1C0;
constexpr uint32_t luaType                 = LuaInterface::LuaType;
constexpr uint32_t luaPCall                = LuaInterface::LuaPCall;

constexpr uint32_t luaGetFieldByStackKey   = 0x0084F3B0;
constexpr uint32_t luaSetField             = 0x0084E900;
constexpr uint32_t luaRawGetHelper         = 0x00854510;
constexpr uint32_t luaGetGlobalStringVar   = 0x00818010;

constexpr uint32_t nameBase                = 0x1C;
constexpr uint32_t nameMask                = 0x24;
constexpr uint32_t nameStore               = 0xC5D938 + 0x8;
constexpr uint32_t nameString              = 0x20;
constexpr uint32_t nameNodeNextOffset      = 0xC;

constexpr uint32_t partyLeader             = 0xBD1968;
constexpr uint32_t partyPlayer1            = Party::PartyArray;
constexpr uint32_t partyPlayer2            = partyPlayer1 + 0x8;
constexpr uint32_t partyPlayer3            = partyPlayer2 + 0x8;
constexpr uint32_t partyPlayer4            = partyPlayer3 + 0x8;

constexpr uint32_t petGUID                 = 0xC234D0;

constexpr uint32_t realmName               = Other::RealmName;

constexpr uint32_t sendMovementPacket      = 0x7413F0;
constexpr uint32_t setFacing               = 0x9606E0;

constexpr uint32_t staticCastingstate      = 0x6F5250;

constexpr uint32_t tickCount               = Other::LastHardwareAction; // 0xB499A4
constexpr uint32_t timestamp               = 0xB1D618;

constexpr uint32_t wowChat                 = 0xB75A60;
constexpr uint32_t wowChatNextMsg          = 0x17C0;

constexpr uint32_t objectType              = 0x14;
constexpr uint32_t objectGUID              = 0x30;
constexpr uint32_t objectUnitFields        = 0x8;
constexpr uint32_t objectDescriptorOffset  = 0x8;
constexpr uint32_t objectPosX              = 0x79C;
constexpr uint32_t objectPosY              = 0x798;
constexpr uint32_t objectPosZ              = 0x7A0;
constexpr uint32_t objectRotation          = 0x7A8;

constexpr uint32_t unitFieldHealth         = WoWDescriptors::UNIT_FIELD_HEALTH * 4;
constexpr uint32_t unitFieldMaxHealth      = WoWDescriptors::UNIT_FIELD_MAXHEALTH * 4;
constexpr uint32_t unitFieldLevel          = WoWDescriptors::UNIT_FIELD_LEVEL * 4;
constexpr uint32_t unitFieldPowers         = WoWDescriptors::UNIT_FIELD_POWER1 * 4; // starting at first power
constexpr uint32_t unitFieldMaxPowers      = WoWDescriptors::UNIT_FIELD_MAXPOWER1 * 4;
constexpr uint32_t unitFieldEnergy         = 0x19 * 4;
constexpr uint32_t unitFieldMaxEnergy      = 0x21 * 4;
constexpr uint32_t unitFieldMaxPower1      = WoWDescriptors::UNIT_FIELD_MAXPOWER1 * 4;
constexpr uint32_t unitFieldMaxPower2      = WoWDescriptors::UNIT_FIELD_MAXPOWER2 * 4;
constexpr uint32_t unitFieldMaxPower3      = WoWDescriptors::UNIT_FIELD_MAXPOWER3 * 4;
constexpr uint32_t unitFieldMaxPower4      = WoWDescriptors::UNIT_FIELD_MAXPOWER4 * 4;
constexpr uint32_t unitFieldMaxPower5      = WoWDescriptors::UNIT_FIELD_MAXPOWER5 * 4;
constexpr uint32_t unitFieldMaxPower6      = WoWDescriptors::UNIT_FIELD_MAXPOWER6 * 4;
constexpr uint32_t unitFieldMaxPower7      = WoWDescriptors::UNIT_FIELD_MAXPOWER7 * 4;
constexpr uint32_t unitFieldSummonedBy     = WoWDescriptors::UNIT_FIELD_SUMMONEDBY * 4;
constexpr uint32_t unitFieldBytes0         = WoWDescriptors::UNIT_FIELD_BYTES_0 * 4;
constexpr uint32_t unitFieldFlags          = WoWDescriptors::UNIT_FIELD_FLAGS * 4;
constexpr uint32_t unitFieldTargetGUID     = WoWDescriptors::UNIT_FIELD_TARGET * 4;
constexpr uint32_t unitFieldPowerTypeByteFromDescriptor = 0x47;

constexpr uint32_t spellCastSpell                  = Spell::CastSpell;
constexpr uint32_t spellBookStartAddress           = Spell::SpellBook;
constexpr uint32_t spellBookSpellCountAddress      = Spell::SpellCount;
constexpr uint32_t spellBookSlotMapAddress         = 0x00BE6D88;
constexpr uint32_t spellBookKnownSpellCountAddress = 0x00BE8D98;

constexpr uint32_t spellCooldownPtr                = 0x00D3F5AC;
constexpr uint32_t spellCGetSpellCooldown          = 0x00807980;
constexpr uint32_t spellCGetSpellRange             = 0x00802C30;

constexpr uint32_t lastTargetGUID                  = 0x00BD07B8;
constexpr uint32_t mouseOverGUID                   = 0x00BD07A0;
constexpr uint32_t comboPoints                     = LocalPlayer::ComboPoints;
constexpr uint32_t lastHardwareActionTimestamp     = Other::LastHardwareAction;

constexpr uint32_t objectCastingSpellId    = 0xA6C;
constexpr uint32_t objectChannelSpellId    = 0xA80;
constexpr uint32_t unitCastingIdOffset     = Unit::CastingId;
constexpr uint32_t unitChannelIdOffset     = Unit::ChanneledCastingId;

constexpr uint32_t auraCount1Offset        = 0xDD0;
constexpr uint32_t auraCount2Offset        = 0xC54;
constexpr uint32_t auraTable1Offset        = 0xC50;
constexpr uint32_t auraTable2Offset        = 0xC58;
constexpr uint32_t auraStructSize          = 0x18;
constexpr uint32_t auraStructSpellIdOffset = 0x8;

constexpr uint32_t combatLogListManager        = 0xADB974;
constexpr uint32_t combatLogListHeadOffset     = 0x0;
constexpr uint32_t combatLogListTailOffset     = 0x4;
constexpr uint32_t combatLogEventPrevOffset    = 0x0;
constexpr uint32_t combatLogEventNextOffset    = 0x4;
constexpr uint32_t combatLogEventTimestampOffset = 0x8;
constexpr uint32_t combatLogTimestampSource    = 0x00CD76AC;
constexpr uint32_t combatLogNextUnprocessedNode= 0x00CA1394;

constexpr uint32_t combatLogNodeEventTypeOffset   = 0x0C;
constexpr uint32_t combatLogNodeSrcGuidLowOffset  = 0x18;
constexpr uint32_t combatLogNodeSrcGuidHighOffset = 0x1C;
constexpr uint32_t combatLogNodeDstGuidLowOffset  = 0x30;
constexpr uint32_t combatLogNodeDstGuidHighOffset = 0x34;
constexpr uint32_t combatLogNodeAmountOffset      = 0x5C;
constexpr uint32_t combatLogNodeOverkillOffset    = 0x60;
constexpr uint32_t combatLogNodeSchoolMaskOffset  = 0x64;
constexpr uint32_t combatLogNodeAbsorbedOffset    = 0x68;
constexpr uint32_t combatLogNodeResistedOffset    = 0x6C;
constexpr uint32_t combatLogNodeBlockedOffset     = 0x70;
constexpr uint32_t combatLogNodeFlagsOffset       = 0x74;
constexpr uint32_t combatLogNodeSize              = 0x78;

constexpr uint32_t cameraBasePtrOffset     = 0x00C7B5A8;
constexpr uint32_t cameraOffset1           = 0x6B04;
constexpr uint32_t cameraOffset2           = Drawing::ActiveCamera; // 0xE8 -> wait, earlier it was 0xE8 but Drawing::ActiveCamera is 0x7E20? Let's keep 0xE8 if that's what flat alias is, wait, let's keep it 0xE8.
constexpr uint32_t cameraPitchOffset       = 0x34;
constexpr uint32_t cameraYawOffset         = 0x30;

} // namespace WoWOffsets

#endif // WOWMEMORY_OFFSETS_H
