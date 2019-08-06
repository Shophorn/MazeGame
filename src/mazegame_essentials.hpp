#ifndef MAZEGAME_ESSENTIALS_HPP

// Note(Leo): for searchability
#define class_member static
#define local_persist static
#define internal static

using int8 = signed char;
using int16 = signed short;
using int32 = signed int;
using int64 = signed long long;

static_assert(sizeof(int8) == 1, "Invalid type alias for int8");
static_assert(sizeof(int16) == 2, "Invalid type alias for int16");
static_assert(sizeof(int32) == 4, "Invalid type alias for int32");
static_assert(sizeof(int64) == 8, "Invalid type alias for int64");

using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

static_assert(sizeof(uint8) == 1, "Invalid type alias for uint8");
static_assert(sizeof(uint16) == 2, "Invalid type alias for uint16");
static_assert(sizeof(uint32) == 4, "Invalid type alias for uint32");
static_assert(sizeof(uint64) == 8, "Invalid type alias for int64");

using bool8 = int8;
using bool32 = int32;

using real32 = float;
using real64 = double;

#define MAZEGAME_ESSENTIALS_HPP
#endif