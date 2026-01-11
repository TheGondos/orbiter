#pragma once
#include <vector>
#include <string>


#define _(S) I18N::GetText(S)
#define _c(C,S) I18N::PGetText(C,S)
#define _n(S,P,N) I18N::NGetText(S,P,N)
#define _nc(C,S,P,N) I18N::PNGetText(C,S,P,N)

namespace I18N {

// Initialize i18n subsystem
void Init(bool notifymissing);

// Get a list of available locales
const std::vector<std::string> &GetLocales();

// Load a locale
void LoadLocale(const char *);

// Translate generic text in singular
const char *GetText(const char *msgid);

// Translate generic text in singular with context
const char *PGetText(const char *msgctx, const char *msgid);

// Translate generic text, possibly plural
const char *NGetText(const char *msgid, const char *msgid_plural, unsigned int n);

// Translate generic text, possibly plural with context
const char *PNGetText(const char *msgctx, const char *msgid, const char *msgid_plural, unsigned int n);

// Count the number of bytes taken by the first n codepoints
std::size_t cp2bytes(const char* s, std::size_t codepoints) noexcept;

}; // namespace
