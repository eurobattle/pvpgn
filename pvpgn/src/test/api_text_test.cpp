/*
 * Unit tests of bnetd/api/text.h: the text that goes in and out of the REST API.
 */
#include "common/setup_before.h"

#include <optional>
#include <random>
#include <string>
#include <string_view>

#include <gtest/gtest.h>
#include <utf8.h>

#include "bnetd/api/text.h"
#include "common/tag.h"
#include "common/setup_after.h"

using namespace std::string_view_literals;
using pvpgn::bnetd::api::parse_clienttag;
using pvpgn::bnetd::api::to_utf8;

namespace {

/* ---- to_utf8: text that already is UTF-8 must come back untouched ---- */

TEST(ToUtf8, KeepsAscii) {
  EXPECT_EQ(to_utf8("That user account is now locked."sv), "That user account is now locked.");
  EXPECT_EQ(to_utf8(" -name- \t users\r\n"sv), " -name- \t users\r\n");
}

TEST(ToUtf8, KeepsEmptyText) {
  EXPECT_EQ(to_utf8(""sv), "");
  EXPECT_EQ(to_utf8(std::string_view()), "");
}

TEST(ToUtf8, NullPointerIsEmptyText) {
  EXPECT_EQ(to_utf8(static_cast<const char*>(nullptr)), "");
}

TEST(ToUtf8, CStringOverloadMatchesStringView) {
  EXPECT_EQ(to_utf8("caf\xc3\xa9"), "caf\xc3\xa9");
  EXPECT_EQ(to_utf8("caf\xe9"), "caf\xc3\xa9");
}

TEST(ToUtf8, KeepsValidMultiByteCharacters) {
  EXPECT_EQ(to_utf8("caf\xc3\xa9"sv), "caf\xc3\xa9");                          // é, 2 bytes
  EXPECT_EQ(to_utf8("\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82"sv),     // Привет
            "\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82");
  EXPECT_EQ(to_utf8("\xe2\x82\xac"sv), "\xe2\x82\xac");                        // €, 3 bytes
  EXPECT_EQ(to_utf8("\xe4\xb8\xad\xe6\x96\x87"sv), "\xe4\xb8\xad\xe6\x96\x87"); // 中文
  EXPECT_EQ(to_utf8("\xf0\x9f\x98\x80"sv), "\xf0\x9f\x98\x80");                // U+1F600, 4 bytes
}

TEST(ToUtf8, KeepsTheFirstAndLastCharacterOfEachLength) {
  EXPECT_EQ(to_utf8("\x7f"sv), "\x7f");                          // U+007F
  EXPECT_EQ(to_utf8("\xc2\x80"sv), "\xc2\x80");                  // U+0080
  EXPECT_EQ(to_utf8("\xdf\xbf"sv), "\xdf\xbf");                  // U+07FF
  EXPECT_EQ(to_utf8("\xe0\xa0\x80"sv), "\xe0\xa0\x80");          // U+0800
  EXPECT_EQ(to_utf8("\xed\x9f\xbf"sv), "\xed\x9f\xbf");          // U+D7FF, last before the surrogates
  EXPECT_EQ(to_utf8("\xee\x80\x80"sv), "\xee\x80\x80");          // U+E000, first after the surrogates
  EXPECT_EQ(to_utf8("\xf0\x90\x80\x80"sv), "\xf0\x90\x80\x80");  // U+10000
  EXPECT_EQ(to_utf8("\xf4\x8f\xbf\xbf"sv), "\xf4\x8f\xbf\xbf");  // U+10FFFF, the last one
}

TEST(ToUtf8, KeepsEmbeddedNul) {
  const std::string_view text("a\0b", 3);
  EXPECT_EQ(to_utf8(text), std::string("a\0b", 3));
}

/* ---- to_utf8: bytes that are not part of a UTF-8 character are read as Latin-1 ---- */

/* What the Latin-1 text `bytes` is in UTF-8, worked out independently of to_utf8(). */
std::string latin1_as_utf8(std::string_view bytes) {
  std::string out;
  for (const char raw : bytes) {
    const unsigned char ch = static_cast<unsigned char>(raw);
    if (ch < 0x80) {
      out += raw;
    } else {
      out += static_cast<char>(0xc0 | (ch >> 6));
      out += static_cast<char>(0x80 | (ch & 0x3f));
    }
  }
  return out;
}

TEST(ToUtf8, ConvertsLatin1Text) {
  EXPECT_EQ(to_utf8("caf\xe9"sv), "caf\xc3\xa9");            // café
  EXPECT_EQ(to_utf8("caf\xe9 temp"sv), "caf\xc3\xa9 temp");
  EXPECT_EQ(to_utf8("\xfc" "ber"sv), "\xc3\xbc" "ber");      // über
  EXPECT_EQ(to_utf8("na\xefve \xa9 \xbd"sv), "na\xc3\xafve \xc2\xa9 \xc2\xbd");  // naïve © ½
}

TEST(ToUtf8, ConvertsEveryLatin1Character) {
  for (int value = 0x80; value < 0x100; value++) {
    const std::string in(1, static_cast<char>(value));
    const std::string out = to_utf8(in);
    SCOPED_TRACE(value);
    ASSERT_EQ(out.size(), 2u);
    // decoded again it is the character with the number of the byte
    EXPECT_EQ(utf8::peek_next(out.begin(), out.end()), static_cast<utf8::utfchar32_t>(value));
  }
}

TEST(ToUtf8, ConvertsBytesThatNeverOccurInUtf8) {
  EXPECT_EQ(to_utf8("a\xff" "b"sv), "a\xc3\xbf" "b");  // ÿ
  EXPECT_EQ(to_utf8("a\xfe" "b"sv), "a\xc3\xbe" "b");  // þ
  EXPECT_EQ(to_utf8("a\xc0" "b"sv), "a\xc3\x80" "b");  // À
  EXPECT_EQ(to_utf8("a\xf8" "b"sv), "a\xc3\xb8" "b");  // ø
}

TEST(ToUtf8, ConvertsAContinuationByteWithoutALeadByte) {
  EXPECT_EQ(to_utf8("a\x80" "b"sv), "a\xc2\x80" "b");
  EXPECT_EQ(to_utf8("a\xbf" "b"sv), "a\xc2\xbf" "b");  // ¿
}

TEST(ToUtf8, ConvertsASequenceCutOffAtTheEnd) {
  EXPECT_EQ(to_utf8("trunc \xc3"sv), "trunc \xc3\x83");                          // 1 of 2 bytes
  EXPECT_EQ(to_utf8("trunc \xe2\x82"sv), "trunc \xc3\xa2\xc2\x82");              // 2 of 3 bytes
  EXPECT_EQ(to_utf8("trunc \xf0\x9f\x98"sv), "trunc \xc3\xb0\xc2\x9f\xc2\x98");  // 3 of 4 bytes
}

TEST(ToUtf8, ConvertsASequenceCutOffInTheMiddle) {
  EXPECT_EQ(to_utf8("a\xe2\x82" "b"sv), "a\xc3\xa2\xc2\x82" "b");
}

/* Each of these decodes to something a strict UTF-8 reader must reject. */
TEST(ToUtf8, TreatsIllFormedSequencesAsLatin1) {
  const std::string_view cases[] = {
      "\xc0\xaf"sv,          // overlong '/'
      "\xc1\xbf"sv,          // overlong U+007F
      "\xe0\x80\xaf"sv,      // overlong '/', 3 bytes
      "\xe0\x9f\xbf"sv,      // overlong U+07FF
      "\xf0\x80\x80\xaf"sv,  // overlong '/', 4 bytes
      "\xf0\x8f\xbf\xbf"sv,  // overlong U+FFFF
      "\xed\xa0\x80"sv,      // U+D800, a UTF-16 surrogate
      "\xed\xbf\xbf"sv,      // U+DFFF, a UTF-16 surrogate
      "\xf4\x90\x80\x80"sv,  // U+110000, past the last character
      "\xf7\xbf\xbf\xbf"sv,  // U+1FFFFF
      "\xf8\x88\x80\x80\x80"sv,  // 5 byte form
  };
  for (const std::string_view bad : cases) {
    const std::string in = "<" + std::string(bad) + ">";
    const std::string out = to_utf8(in);
    SCOPED_TRACE(testing::PrintToString(std::string(bad)));
    EXPECT_TRUE(utf8::is_valid(out));
    EXPECT_EQ(out, latin1_as_utf8(in));
    // in particular the overlong '/' must not come out as a '/'
    EXPECT_EQ(out.find('/'), std::string::npos);
  }
}

/* Text of two kinds in one line, e.g. a list of names: each part is read as what it is. */
TEST(ToUtf8, KeepsValidUtf8NextToLatin1) {
  EXPECT_EQ(to_utf8("caf\xc3\xa9 \xe9"sv), "caf\xc3\xa9 \xc3\xa9");
  EXPECT_EQ(to_utf8("Jos\xc3\xa9" "Andr\xe9"sv), "Jos\xc3\xa9" "Andr\xc3\xa9");
  EXPECT_EQ(to_utf8("\xe9 \xe2\x82\xac \xe9"sv), "\xc3\xa9 \xe2\x82\xac \xc3\xa9");
}

/* bnetd cuts text by bytes: only the character that was cut may come out wrong. */
TEST(ToUtf8, KeepsTheTextBeforeACutOffCharacter) {
  EXPECT_EQ(to_utf8("Jos\xc3\xa9\xe2\x82"sv), "Jos\xc3\xa9\xc3\xa2\xc2\x82");
  EXPECT_EQ(to_utf8("\xd0\x9f\xd1\x80\xd0\xb8\xd0"sv), "\xd0\x9f\xd1\x80\xd0\xb8\xc3\x90");  // При + half a letter
}

/* The valid character after a bad lead byte is kept too. */
TEST(ToUtf8, ResumesRightAfterABadByte) {
  EXPECT_EQ(to_utf8("\xe2\xc3\xa9"sv), "\xc3\xa2\xc3\xa9");
}

/*
 * What to_utf8() has to return, worked out independently of it: going through
 * the text, a complete valid UTF-8 character is kept and any other byte is
 * converted from Latin-1.
 */
std::string expected_utf8(std::string_view in) {
  std::string out;
  for (std::size_t i = 0; i < in.size();) {
    std::size_t kept = 0;
    for (std::size_t len = 1; len <= 4 && i + len <= in.size() && !kept; len++) {
      const std::string_view part = in.substr(i, len);
      if (utf8::is_valid(part) && utf8::distance(part.begin(), part.end()) == 1)
        kept = len;
    }
    if (kept) {
      out += in.substr(i, kept);
      i += kept;
    } else {
      out += latin1_as_utf8(in.substr(i, 1));
      i++;
    }
  }
  return out;
}

/* ---- to_utf8: whatever goes in, valid UTF-8 comes out (the JSON writer depends on it) ---- */

TEST(ToUtf8, EverySingleByteGivesValidUtf8) {
  for (int value = 0; value < 256; value++) {
    const std::string in(1, static_cast<char>(value));
    const std::string out = to_utf8(in);
    SCOPED_TRACE(value);
    EXPECT_TRUE(utf8::is_valid(out));
    EXPECT_EQ(out, value < 0x80 ? in : latin1_as_utf8(in));
  }
}

TEST(ToUtf8, EveryPairOfBytesGivesValidUtf8) {
  for (int first = 0; first < 256; first++) {
    for (int second = 0; second < 256; second++) {
      const std::string in{static_cast<char>(first), static_cast<char>(second)};
      const std::string out = to_utf8(in);
      ASSERT_TRUE(utf8::is_valid(out)) << "bytes " << first << " " << second;
      ASSERT_EQ(out, expected_utf8(in)) << "bytes " << first << " " << second;
      if (utf8::is_valid(in))
        ASSERT_EQ(out, in) << "bytes " << first << " " << second;
    }
  }
}

TEST(ToUtf8, RandomBytesGiveValidUtf8) {
  std::mt19937 random(20261116);  // fixed seed: the same texts on every run
  std::uniform_int_distribution<int> byte(0, 255);
  std::uniform_int_distribution<int> length(0, 64);

  for (int round = 0; round < 20000; round++) {
    std::string in(static_cast<std::size_t>(length(random)), '\0');
    for (char& ch : in)
      ch = static_cast<char>(byte(random));
    const std::string out = to_utf8(in);
    ASSERT_TRUE(utf8::is_valid(out)) << "round " << round;
    ASSERT_EQ(out, expected_utf8(in)) << "round " << round;
    if (utf8::is_valid(in))
      ASSERT_EQ(out, in) << "round " << round;
  }
}

/* Converted text is UTF-8, so converting it again must not change it a second time. */
TEST(ToUtf8, IsStableWhenAppliedTwice) {
  const std::string once = to_utf8("caf\xe9 \xf0\x9f\x98 \xc3\xa9"sv);
  EXPECT_EQ(to_utf8(once), once);
}

/* ---- parse_clienttag ---- */

TEST(ParseClienttag, KnowsTheClientTags) {
  EXPECT_EQ(parse_clienttag("W3XP"), std::optional<pvpgn::t_clienttag>(CLIENTTAG_WAR3XP_UINT));
  EXPECT_EQ(parse_clienttag("WAR3"), std::optional<pvpgn::t_clienttag>(CLIENTTAG_WARCRAFT3_UINT));
  EXPECT_EQ(parse_clienttag("STAR"), std::optional<pvpgn::t_clienttag>(CLIENTTAG_STARCRAFT_UINT));
  EXPECT_EQ(parse_clienttag("CHAT"), std::optional<pvpgn::t_clienttag>(CLIENTTAG_BNCHATBOT_UINT));
}

TEST(ParseClienttag, IgnoresCase) {
  EXPECT_EQ(parse_clienttag("w3xp"), parse_clienttag("W3XP"));
  EXPECT_EQ(parse_clienttag("War3"), parse_clienttag("WAR3"));
}

TEST(ParseClienttag, RejectsUnknownTags) {
  EXPECT_EQ(parse_clienttag("XXXX"), std::nullopt);
  EXPECT_EQ(parse_clienttag("W3XQ"), std::nullopt);
  EXPECT_EQ(parse_clienttag("1234"), std::nullopt);
}

TEST(ParseClienttag, RejectsTheWrongLength) {
  EXPECT_EQ(parse_clienttag(""), std::nullopt);
  EXPECT_EQ(parse_clienttag("W3"), std::nullopt);
  EXPECT_EQ(parse_clienttag("W3X"), std::nullopt);
  EXPECT_EQ(parse_clienttag("W3XPP"), std::nullopt);
  EXPECT_EQ(parse_clienttag("W3XP "), std::nullopt);
}

TEST(ParseClienttag, RejectsAnythingButPrintableAscii) {
  EXPECT_EQ(parse_clienttag("W3 P"), std::nullopt);
  EXPECT_EQ(parse_clienttag("W3X\n"), std::nullopt);
  EXPECT_EQ(parse_clienttag("W3X\x7f"), std::nullopt);
  EXPECT_EQ(parse_clienttag("\xff\xff\xff\xff"), std::nullopt);
  EXPECT_EQ(parse_clienttag("W3\xc3\xa9"), std::nullopt);
  EXPECT_EQ(parse_clienttag(std::string_view("W3\0P", 4)), std::nullopt);
  EXPECT_EQ(parse_clienttag(std::string_view("\0\0\0\0", 4)), std::nullopt);
}

}  // namespace
