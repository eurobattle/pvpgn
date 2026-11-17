/*
 * Unit tests of bnetd/api/dto/RawCommandDto.h: what POST /rawcmd accepts.
 */
#include "common/setup_before.h"

#include <string>

#include <gtest/gtest.h>

#include "bnetd/api/dto/RawCommandDto.h"
#include "common/field_sizes.h"
#include "common/setup_after.h"

using pvpgn::bnetd::api::RawCommandRequest;
using pvpgn::bnetd::api::validate;

namespace {

RawCommandRequest command_of_length(std::size_t size) {
  return RawCommandRequest{"/" + std::string(size - 1, 'a'), std::nullopt};
}

/* The legacy handlers copy at most MAX_MESSAGE_LEN - 1 bytes into a
 * MAX_MESSAGE_LEN buffer, so the NUL only fits below that length. */
TEST(RawCommandValidate, AcceptsLongestCommandThatKeepsItsTerminator) {
  EXPECT_FALSE(validate(command_of_length(MAX_MESSAGE_LEN - 2)));
}

TEST(RawCommandValidate, RejectsCommandsAtTheLegacyBufferBoundary) {
  EXPECT_TRUE(validate(command_of_length(MAX_MESSAGE_LEN - 1)));
  EXPECT_TRUE(validate(command_of_length(MAX_MESSAGE_LEN)));
  EXPECT_TRUE(validate(command_of_length(MAX_MESSAGE_LEN + 1)));
}

TEST(RawCommandValidate, RequiresLeadingSlash) {
  EXPECT_TRUE(validate(RawCommandRequest{"time", std::nullopt}));
  EXPECT_TRUE(validate(RawCommandRequest{"/", std::nullopt}));
  EXPECT_FALSE(validate(RawCommandRequest{"/time", std::nullopt}));
}

TEST(RawCommandValidate, RejectsControlCharacters) {
  EXPECT_TRUE(validate(RawCommandRequest{"/time\n/lockacct x", std::nullopt}));
  EXPECT_TRUE(validate(RawCommandRequest{std::string("/time\0x", 7), std::nullopt}));
}

TEST(RawCommandValidate, ChecksClienttag) {
  EXPECT_FALSE(validate(RawCommandRequest{"/games", "W3XP"}));
  EXPECT_TRUE(validate(RawCommandRequest{"/games", "NOPE"}));
}

}  // namespace
