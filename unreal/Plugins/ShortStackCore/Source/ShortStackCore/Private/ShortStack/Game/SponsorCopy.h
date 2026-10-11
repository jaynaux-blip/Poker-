#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

// The sponsor book's words (next_segment_spec §3, N8): every letter body, subject and spoken line the book writes, in
// one table, so the copy lint reads exactly what ships. Private to core (Sponsor.cpp, SponsorCopy.cpp). core-sponsor.
namespace ss
{
namespace sponsor
{
namespace sponsor_copy
{
/** Kinds of line: a letter's body (at most 600 characters filled), its subject (200), a text or spoken line (200). */
enum Kind : int
{
	Body = 0,
	Subject = 1,
	Spoken = 2,
};
/**
 * The template for Key in Kind ("strike:embercrest"), falling back a part at a time ("strike"); "" when there's none.
 * Templates use {Brand} {Short} {Rep} {Why} {Amount} {Until} {Cure} {Missed} {Period} {Rival} {Item} {Patch}.
 */
std::string Text(int Kind, const std::string& Key);
/** Every (key, template) of a kind, in table order. */
const std::vector<std::pair<std::string, std::string>>& All(int Kind);
/** Template with each {Var} Vars knows filled; unknown ones stay as written. */
std::string Fill(const std::string& Template, const std::map<std::string, std::string>& Vars);
/** The variables with samples of realistic length (the lint fills every template from these). */
std::map<std::string, std::string> Samples();
} // namespace sponsor_copy
} // namespace sponsor
} // namespace ss
