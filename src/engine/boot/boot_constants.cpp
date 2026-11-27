#include "boot_constants.h"

const char *boot_mode_name(EBootType mode) {
	switch (mode) {
		case DB_BOOT_WLD: return "world";
		case DB_BOOT_MOB: return "mob";
		case DB_BOOT_OBJ: return "obj";
		case DB_BOOT_ZON: return "ZON";
			//case DB_BOOT_SHP:    return "SHP";
		case DB_BOOT_HLP: return "HLP";
		case DB_BOOT_TRG: return "trg";
		default: return "unknown";
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
