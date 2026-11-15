#include <gtest/gtest.h>

#include "insert-mode-policy.h"

namespace goxvi_mac {
namespace {

TEST(InsertModePolicyTest, DirectByDefault) {
  EXPECT_EQ(chooseInsertMode("ru.keepcoder.Telegram", true), InsertMode::Direct);
  EXPECT_EQ(chooseInsertMode("com.microsoft.VSCode", true), InsertMode::Direct);
  EXPECT_EQ(chooseInsertMode("com.google.Chrome", true), InsertMode::Direct);
}

TEST(InsertModePolicyTest, TerminalsUseMarked) {
  EXPECT_EQ(chooseInsertMode("com.googlecode.iterm2", true), InsertMode::Marked);
  EXPECT_EQ(chooseInsertMode("com.apple.Terminal", true), InsertMode::Marked);
}

TEST(InsertModePolicyTest, UnreadableSelectionAlwaysMarked) {
  EXPECT_EQ(chooseInsertMode("ru.keepcoder.Telegram", false), InsertMode::Marked);
  EXPECT_EQ(chooseInsertMode("", false), InsertMode::Marked);
}

TEST(InsertModePolicyTest, EmptyOrUnknownBundleIsDirect) {
  EXPECT_EQ(chooseInsertMode("", true), InsertMode::Direct);
  EXPECT_EQ(chooseInsertMode("com.example.Unknown", true), InsertMode::Direct);
}

TEST(InsertModePolicyTest, MatchIsExact) {
  EXPECT_EQ(chooseInsertMode("com.googlecode.iterm2.beta", true), InsertMode::Direct);
  EXPECT_EQ(chooseInsertMode("COM.APPLE.TERMINAL", true), InsertMode::Direct);
  EXPECT_EQ(chooseInsertMode("com.apple.Termina", true), InsertMode::Direct);
}

}  // namespace
}  // namespace goxvi_mac
