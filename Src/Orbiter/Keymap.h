// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// =======================================================================
// key mapping interface
// =======================================================================

#ifndef __KEYMAP_H
#define __KEYMAP_H

#include "Orbitersdk.h"

// key modifier list
#define KMOD_LSHIFT 0x0100
#define KMOD_RSHIFT 0x0200
#define KMOD_SHIFT  (KMOD_LSHIFT|KMOD_RSHIFT)
#define KMOD_LCTRL  0x0400
#define KMOD_RCTRL  0x0800
#define KMOD_CTRL   (KMOD_LCTRL|KMOD_RCTRL)
#define KMOD_LALT   0x1000
#define KMOD_RALT   0x2000
#define KMOD_ALT    (KMOD_LALT|KMOD_RALT)

class Keymap {
public:
	Keymap ();

	void SetDefault ();
	// set orbiter-default key mapping

	bool Read (const char *fname);
	// parse keymap table from file

	void Write (const char *fname);
	// write keymap table to file

	bool IsLogicalKey (char *kstate, int lfunc) const;
	// return true if logical function lfunc is selected by key state kstate

	bool IsLogicalKey (int key, char *kstate, int lfunc) const;
	// return true if logical function lfunc is selected by key and kstate
	// (kstate only used for modifier keys)
	// if clearkey == true then key is set to 0 on return, if a match is found

	bool ScanStr (char *cbuf, uint16_t &key) const;
	char *PrintStr (char *cbuf, uint16_t key) const;
private:
	bool IsMatchingModifier (char *kstate, int key) const;


	uint16_t func[LKEY_COUNT];
	// list of logical function keys
	// LOBYTE: key id; HIBYTE: modifier (see modifier list)
};

#endif // !__KEYMAP_H