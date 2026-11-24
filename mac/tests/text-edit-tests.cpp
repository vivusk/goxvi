#include <gtest/gtest.h>

#include "text-edit.h"

namespace goxvi_mac {
namespace {

TEST(TextEditTest, AppendReplacesOnlyChangedTail) {
  const TextEdit edit = diffTextEdit(L"vie", L"viê");
  EXPECT_EQ(edit.deleteCount, 1u);
  EXPECT_EQ(edit.insert, L"ê");
}

TEST(TextEditTest, PureAppendDeletesNothing) {
  const TextEdit edit = diffTextEdit(L"tr", L"tru");
  EXPECT_EQ(edit.deleteCount, 0u);
  EXPECT_EQ(edit.insert, L"u");
}

TEST(TextEditTest, PureDeleteInsertsNothing) {
  const TextEdit edit = diffTextEdit(L"việt", L"việ");
  EXPECT_EQ(edit.deleteCount, 1u);
  EXPECT_TRUE(edit.insert.empty());
}

TEST(TextEditTest, FullRewriteForShortcutExpansion) {
  const TextEdit edit = diffTextEdit(L"vn", L"Việt Nam");
  EXPECT_EQ(edit.deleteCount, 2u);
  EXPECT_EQ(edit.insert, L"Việt Nam");
}

TEST(TextEditTest, EqualTextsGiveEmptyEdit) {
  EXPECT_TRUE(diffTextEdit(L"việt", L"việt").empty());
  EXPECT_TRUE(diffTextEdit(L"", L"").empty());
}

TEST(TextEditTest, EmptyOnScreenInsertsWholeTarget) {
  const TextEdit edit = diffTextEdit(L"", L"a");
  EXPECT_EQ(edit.deleteCount, 0u);
  EXPECT_EQ(edit.insert, L"a");
}

TEST(TextEditTest, EmptyTargetDeletesEverything) {
  const TextEdit edit = diffTextEdit(L"việt", L"");
  EXPECT_EQ(edit.deleteCount, 4u);
  EXPECT_TRUE(edit.insert.empty());
  EXPECT_FALSE(edit.empty());
}

}  // namespace
}  // namespace goxvi_mac
