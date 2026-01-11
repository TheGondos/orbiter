#pragma once
#include "i18n.h"
#include <vector>
#include <forward_list>
#include <unordered_map>
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cctype>
#include <cstring>
#include <regex>
#include <filesystem>

namespace I18N {

// -------------------- Structures --------------------
struct Translation {
    std::vector<std::string> plurals; // plurals[0] = singular, plurals[1..] = plural forms
};

struct PluralRule {
    int nplurals;
    int (*get_index)(unsigned int n);
};

// -------------------- Global state --------------------
static std::string g_currentLocale;
static std::vector<std::string> g_locales;
static bool g_notifyMissing = true;
static PluralRule g_currentPluralRule = {2, [](unsigned int n){ return n != 1 ? 1 : 0; }};

// -------------------- Translation storage --------------------
static std::forward_list<std::string> g_translationKeys; // owns keys permanently
static std::unordered_map<std::string_view, Translation> g_translations;

// -------------------- Resizable buffer for temporary keys --------------------
static std::vector<char> g_keyBuffer(256);

// -------------------- Helpers --------------------
static std::string RemoveSpaces(const std::string &s) {
    std::string out; out.reserve(s.size());
    for (char c : s) if (!isspace(static_cast<unsigned char>(c))) out += static_cast<char>(tolower(c));
    return out;
}

static void SelectPluralRuleFromHeader(const std::string &header) {
    std::string normalized = RemoveSpaces(header);

    static const std::vector<std::pair<std::string, PluralRule>> knownRules = {
        {"nplurals=2;plural=(n!=1)", {2, [](unsigned int n){ return n != 1 ? 1 : 0; }}}, // English
        {"nplurals=2;plural=(n>1)",  {2, [](unsigned int n){ return n > 1 ? 1 : 0; }}}, // French
		{"nplurals=1;plural=0",      {1, [](unsigned int n){ return 0;}}}, // Japanese
        {"nplurals=3;plural=(n%10==1&&n%100!=11?0:n%10>=2&&n%10<=4&&(n%100<10||n%100>=20)?1:2)",
                                         {3, [](unsigned int n){ return (n%10==1 && n%100!=11) ? 0 :
                                                                       (n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20)) ? 1 : 2; }}}, // Russian
    };

    for (auto &rule : knownRules) {
        if (normalized.find(rule.first) != std::string::npos) {
            g_currentPluralRule = rule.second;
            return;
        }
    }
    throw std::runtime_error("Unsupported plural form in PO file: " + header);
}

static std::string_view MakeKeyView(const char *ctx, const char *id) {
    size_t len_ctx = strlen(ctx);
    size_t len_id = strlen(id);
    size_t total = len_ctx + 1 + len_id; // +1 for '\x04'
    if (g_keyBuffer.size() <= total) g_keyBuffer.resize(total + 16);
    char *buf = g_keyBuffer.data();
    memcpy(buf, ctx, len_ctx);
    buf[len_ctx] = '\x04';
    memcpy(buf + len_ctx + 1, id, len_id);
    return std::string_view(buf, total);
}

// -------------------- PO string unescape --------------------
static char HexDigitToValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    throw std::runtime_error(std::string("Invalid hex digit: ") + c);
}

static unsigned int ParseHex(const std::string &s, size_t &i, int digits) {
    unsigned int val = 0;
    for (int d = 0; d < digits && i < s.size(); ++d, ++i) val = (val << 4) | HexDigitToValue(s[i]);
    return val;
}

static unsigned int ParseOctal(const std::string &s, size_t &i, int maxDigits = 3) {
    unsigned int val = 0;
    int count = 0;
    while (count < maxDigits && i < s.size() && s[i] >= '0' && s[i] <= '7') {
        val = (val << 3) | (s[i] - '0');
        ++i; ++count;
    }
    return val;
}

static std::string UnescapePOString(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    size_t i = 0;
    if (!s.empty() && s.front() == '"' && s.back() == '"') i = 1;
    size_t end = (!s.empty() && s.back() == '"') ? s.size() - 1 : s.size();

    while (i < end) {
        char c = s[i];
        if (c == '\\' && i + 1 < end) {
            ++i;
            char next = s[i];
            switch (next) {
                case 'n': { out += '\n'; ++i; break; }
                case 'r': { out += '\r'; ++i; break; }
                case 't': { out += '\t'; ++i; break; }
                case '\\': { out += '\\'; ++i; break; }
                case '"': { out += '"'; ++i; break; }
                case 'x': {
                    ++i;
                    unsigned int val = ParseHex(s, i, 2);
                    out += static_cast<char>(val);
                    break;
                }
                case 'u': {
                    ++i;
                    unsigned int val = ParseHex(s, i, 4);
                    if (val <= 0x7F) out += static_cast<char>(val);
                    else if (val <= 0x7FF) {
                        out += static_cast<char>(0xC0 | ((val >> 6) & 0x1F));
                        out += static_cast<char>(0x80 | (val & 0x3F));
                    } else {
                        out += static_cast<char>(0xE0 | ((val >> 12) & 0x0F));
                        out += static_cast<char>(0x80 | ((val >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (val & 0x3F));
                    }
                    break;
                }
                case 'U': {
                    ++i;
                    unsigned int val = ParseHex(s, i, 8);
                    if (val <= 0x7F) out += static_cast<char>(val);
                    else if (val <= 0x7FF) {
                        out += static_cast<char>(0xC0 | ((val >> 6) & 0x1F));
                        out += static_cast<char>(0x80 | (val & 0x3F));
                    } else if (val <= 0xFFFF) {
                        out += static_cast<char>(0xE0 | ((val >> 12) & 0x0F));
                        out += static_cast<char>(0x80 | ((val >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (val & 0x3F));
                    } else {
                        out += static_cast<char>(0xF0 | ((val >> 18) & 0x07));
                        out += static_cast<char>(0x80 | ((val >> 12) & 0x3F));
                        out += static_cast<char>(0x80 | ((val >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (val & 0x3F));
                    }
                    break;
                }
                default: { // octal or unknown escape
                    if (next >= '0' && next <= '7') {
                        unsigned int val = ParseOctal(s, i);
                        out += static_cast<char>(val);
                    } else {
                        out += next;
                        ++i;
                    }
                    break;
                }
            }
        } else {
            out += c;
            ++i;
        }
    }
    return out;
}

bool ends_with(const std::string& s, const std::string& suffix)
{
    if (suffix.size() > s.size())
        return false;

    return s.compare(
        s.size() - suffix.size(),
        suffix.size(),
        suffix
    ) == 0;
}

bool starts_with(const std::string& s, const std::string& prefix)
{
    return s.size() >= prefix.size() &&
           s.compare(0, prefix.size(), prefix) == 0;
}

std::string extract_po_string(const std::string& line)
{
    auto first = line.find('"');
    if (first == std::string::npos)
        return {};

    auto last = line.rfind('"');
    if (last <= first)
        return {};

    return UnescapePOString(
        line.substr(first + 1, last - first - 1)
    );
}

void LoadPO(const char *filename);

void LoadLocale(const char *locale)
{
    g_currentLocale = locale;
    g_translations.clear();
    g_translationKeys.clear();

	std::string suffix = std::string(".") + locale + ".po";
	for (const auto& entry : std::filesystem::directory_iterator("i18n"))
    {
        if (!entry.is_regular_file())
            continue;

        const std::string name = entry.path().filename().string();
		if(ends_with(name, suffix)) {
			LoadPO(entry.path().string().c_str());
		}
    }
}
// -------------------- API --------------------
void Init(bool notifymissing) {
    g_notifyMissing = notifymissing;
    g_locales.clear();
    g_locales.push_back("en_US");
    g_locales.push_back("fr_FR");
}

const std::vector<std::string> &GetLocales() { return g_locales; }

void LoadPO(const char *filename) {
    std::ifstream f(filename);
    if (!f.is_open()) { if (g_notifyMissing) std::cerr << "Locale file not found: " << filename << "\n"; return; }

    // --- Parse header ---
    std::string line, header;
    bool inHeader = false;
    while (std::getline(f, line)) {
        if (line.substr(0,6)=="msgstr"){ inHeader=true; header+=line.substr(6); }
        else if (inHeader && !line.empty() && line.front()=='"') header+=line;
        else if (inHeader) break;
    }
    if (!header.empty()) SelectPluralRuleFromHeader(header);

    f.clear(); f.seekg(0);

    std::string raw_msgctxt, raw_msgid, raw_msgid_plural;
    Translation currentTranslation;
    std::string *currentTarget = nullptr;

    while (std::getline(f, line)) {
        if (line.empty() || line[0]=='#') { // end entry
            if (!raw_msgid.empty()) {
                std::string msgctxt = UnescapePOString(raw_msgctxt);
                std::string msgid = UnescapePOString(raw_msgid);
                std::string key = msgctxt.empty() ? msgid : msgctxt + '\x04' + msgid;
                g_translationKeys.push_front(std::move(key));
                g_translations[std::string_view(g_translationKeys.front())] = currentTranslation;

                raw_msgctxt.clear(); raw_msgid.clear(); raw_msgid_plural.clear();
                currentTranslation.plurals.clear(); currentTarget=nullptr;
            }
            continue;
        }

        if (starts_with(line,"msgctxt")) raw_msgctxt=line.substr(9), currentTarget=nullptr;
        else if (starts_with(line,"msgid_plural")) raw_msgid_plural=extract_po_string(line), currentTarget=nullptr;
        else if (starts_with(line,"msgid")) raw_msgid=extract_po_string(line), currentTarget=nullptr;
        else if (starts_with(line,"msgstr ")) { currentTranslation.plurals.resize(1); currentTranslation.plurals[0]=UnescapePOString(extract_po_string(line)); currentTarget=&currentTranslation.plurals[0]; }
        else if (starts_with(line,"msgstr[")) {
            size_t idx_end=line.find(']',6);
            if (idx_end!=std::string::npos) {
                int idx=std::stoi(line.substr(6,idx_end-6));
                if ((size_t)idx>=currentTranslation.plurals.size()) currentTranslation.plurals.resize(idx+1);
                currentTranslation.plurals[idx]=UnescapePOString(extract_po_string(line));
                currentTarget=&currentTranslation.plurals[idx];
            }
        }
        else if (!line.empty() && line.front()=='"' && currentTarget) *currentTarget += UnescapePOString(line);
    }

    if (!raw_msgid.empty()) { // last entry
        std::string msgctxt = UnescapePOString(raw_msgctxt);
        std::string msgid = UnescapePOString(raw_msgid);
        std::string key = msgctxt.empty() ? msgid : msgctxt + '\x04' + msgid;
        g_translationKeys.push_front(std::move(key));
        g_translations[std::string_view(g_translationKeys.front())] = currentTranslation;
    }
}

// -------------------- GetText API --------------------
const char *GetText(const char *msgid);
const char *PGetText(const char *msgctx, const char *msgid);
const char *NGetText(const char *msgid, const char *msgid_plural, unsigned int n);
const char *PNGetText(const char *msgctx, const char *msgid, const char *msgid_plural, unsigned int n);

const char *GetText(const char *msgid) { return PGetText(nullptr,msgid); }

const char *PGetText(const char *msgctx, const char *msgid) {
    if (!msgctx || msgctx[0]=='\0') {
        auto it=g_translations.find(msgid);
        if(it!=g_translations.end()&&!it->second.plurals.empty()&&!it->second.plurals[0].empty()) return it->second.plurals[0].c_str();
        //if(g_notifyMissing) std::cerr << "Missing translation: " << msgid << "\n";
        return msgid;
    }
    std::string_view key=MakeKeyView(msgctx,msgid);
    auto it=g_translations.find(key);
    if(it!=g_translations.end()&&!it->second.plurals.empty()&&!it->second.plurals[0].empty()) return it->second.plurals[0].c_str();
    //if(g_notifyMissing) std::cerr << "Missing translation: " << key << "\n";
    return msgid;
}

const char *NGetText(const char *msgid,const char *msgid_plural,unsigned int n) { return PNGetText(nullptr,msgid,msgid_plural,n); }

const char *PNGetText(const char *msgctx,const char *msgid,const char *msgid_plural,unsigned int n) {
    std::string_view key = (!msgctx || msgctx[0]=='\0') ? std::string_view(msgid) : MakeKeyView(msgctx,msgid);
    auto it=g_translations.find(key);
    if(it!=g_translations.end()&&!it->second.plurals.empty()) {
        int idx=g_currentPluralRule.get_index(n);
        if((size_t)idx>=it->second.plurals.size()) idx=it->second.plurals.size()-1;
        return it->second.plurals[idx].c_str();
    }
    if(g_notifyMissing) std::cerr << "Missing plural translation: " << key << "\n";
    return (n==1)?msgid:msgid_plural;
}

std::size_t cp2bytes(const char* s, std::size_t max_codepoints) noexcept
{
    std::size_t bytes = 0;
    std::size_t count = 0;

    while (s[bytes] && count < max_codepoints) {
        const unsigned char c =
            static_cast<unsigned char>(s[bytes]);

        std::size_t advance =
            (c & 0x80) == 0x00 ? 1 :
            (c & 0xE0) == 0xC0 ? 2 :
            (c & 0xF0) == 0xE0 ? 3 :
            (c & 0xF8) == 0xF0 ? 4 :
            0;

        if (advance == 0) break; // invalid UTF-8 start byte

        bytes += advance;
        ++count;
    }

    return bytes;
}

} // namespace I18N
