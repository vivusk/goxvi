#include <gtest/gtest.h>

#include "letter-handoff.h"

namespace goxvi_mac {
namespace {

TEST(LetterHandoffTest, FreshLetterIsTakenOnce) {
  LetterHandoff h;
  h.put(L'd', 10.0);
  EXPECT_EQ(h.take(10.3), L'd');
  EXPECT_FALSE(h.take(10.4).has_value());
}

TEST(LetterHandoffTest, StaleOrEmptyGivesNothing) {
  LetterHandoff h;
  EXPECT_FALSE(h.take(1.0).has_value());
  h.put(L'd', 10.0);
  EXPECT_FALSE(h.take(10.0 + LetterHandoff::kMaxAgeSeconds + 0.1).has_value());
  h.put(L'd', 10.0);
  EXPECT_FALSE(h.take(9.0).has_value());  // clock went back: never trust it
}

}  // namespace
}  // namespace goxvi_mac
