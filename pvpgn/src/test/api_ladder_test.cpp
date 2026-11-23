/*
 * Unit tests of bnetd/api/ladder.h: the names of the matchmaking queues.
 */
#include "common/setup_before.h"

#include <optional>
#include <set>
#include <string_view>

#include <gtest/gtest.h>

#include "bnetd/api/ladder.h"
#include "common/setup_after.h"

using pvpgn::bnetd::api::parse_queue;
using pvpgn::bnetd::api::queue_name;

namespace {

TEST(LadderQueue, NamesTheClassicQueues) {
  EXPECT_EQ(queue_name(ANONGAME_TYPE_1V1), "1v1");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_2V2), "2v2");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_3V3), "3v3");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_4V4), "4v4");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_SMALL_FFA), "ffa");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_AT_2V2), "at2v2");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_TEAM_FFA), "atffa");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_AT_3V3), "at3v3");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_AT_4V4), "at4v4");
}

TEST(LadderQueue, NamesTheOtherQueues) {
  EXPECT_EQ(queue_name(ANONGAME_TYPE_TY), "tournament");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_5V5), "5v5");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_6V6), "6v6");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_2V2V2), "2v2v2");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_3V3V3), "3v3v3");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_4V4V4), "4v4v4");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_2V2V2V2), "2v2v2v2");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_3V3V3V3), "3v3v3v3");
  EXPECT_EQ(queue_name(ANONGAME_TYPE_AT_2V2V2), "at2v2v2");
}

TEST(LadderQueue, HasNoNameOutsideTheQueues) {
  EXPECT_EQ(queue_name(-1), std::nullopt);
  EXPECT_EQ(queue_name(ANONGAME_TYPES), std::nullopt);
  EXPECT_EQ(queue_name(1000), std::nullopt);
}

TEST(LadderQueue, EveryQueueHasItsOwnName) {
  std::set<std::string_view> names;
  for (int queue = 0; queue < ANONGAME_TYPES; queue++) {
    const std::optional<std::string_view> name = queue_name(queue);
    ASSERT_TRUE(name) << "queue " << queue;
    EXPECT_FALSE(name->empty()) << "queue " << queue;
    EXPECT_TRUE(names.insert(*name).second) << "queue " << queue << " shares the name " << *name;
  }
}

TEST(LadderQueue, ParsesWhatItNames) {
  for (int queue = 0; queue < ANONGAME_TYPES; queue++)
    EXPECT_EQ(parse_queue(*queue_name(queue)), queue) << "queue " << queue;
}

TEST(LadderQueue, ParsesOnlyExactNames) {
  EXPECT_EQ(parse_queue(""), std::nullopt);
  EXPECT_EQ(parse_queue("1V1"), std::nullopt);
  EXPECT_EQ(parse_queue(" 1v1"), std::nullopt);
  EXPECT_EQ(parse_queue("1v1 "), std::nullopt);
  EXPECT_EQ(parse_queue("1v"), std::nullopt);
  EXPECT_EQ(parse_queue("7v7"), std::nullopt);
  EXPECT_EQ(parse_queue("PG 1v1"), std::nullopt);
  EXPECT_EQ(parse_queue(std::string_view("1v1\0", 4)), std::nullopt);
}

}  // namespace
