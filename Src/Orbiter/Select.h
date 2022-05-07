#pragma once

#include "OrbiterAPI.h"

#define ITEM_SUBMENU 0x01
#define ITEM_NOHILIGHT 0x02
#define ITEM_SEPARATOR 0x04

struct SelectEntry {
    int m_Flags;
    std::string m_Text;
    std::list<SelectEntry> m_SubEntries;
};

class Select : public GUIElement {
public:
	typedef bool (*Callbk)(Select*, int, const char*, void*);
    Select(const std::string &name);
    void Show() override;
    static const std::string etype;
    bool m_Opened;
    std::string m_Title;
    std::list<SelectEntry> m_RootMenu;
    std::list<SelectEntry> *m_CurrentEntry;
    Callbk m_cbSubmenu;
    Callbk m_cbEnter;
    void *m_UserData;

    void Open(const char *_title = 0, Callbk submenu_cbk = 0, Callbk enter_cbk = 0, void *_userdata = 0, int cntx = -1, int cnty = -1);
    void Append(const char *str, int flags = 0);
	void AppendSeparator ();
    void DrawMenu(std::list<SelectEntry> &entries);
};

class InputBox : public GUIElement {
public:
	typedef bool (*Callbk)(InputBox*, const char*, void*);
    InputBox(const std::string &name);
    void Show() override;
    static const std::string etype;
    bool m_Opened;
    std::string m_Title;
    Callbk m_cbEnter;
    Callbk m_cbCancel;
    void *m_UserData;
    char m_Buf[128];

	void Open (const char *_title = 0, const char *_buf = 0, int _vislen = 20,
		Callbk cbk = 0, void *_userdata = 0, int cntx = -1, int cnty = -1);

	bool OpenEx (const char *_title = 0, const char *_buf = 0, int _vislen = 20,
		Callbk enter_cbk = 0, Callbk cancel_cbk = 0, void *_userdata = 0,
		int flags = 0, int cntx = -1, int cnty = -1);
};
