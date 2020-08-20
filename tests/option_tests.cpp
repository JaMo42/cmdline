#include "gtest/gtest.h"
#include "cmdline.h"

template<typename T, std::size_t N>
auto size(T (&)[N]) { return N; }

struct ParserWrapper : public cmdline::ArgumentParser {
  auto &get_options() {
    return m_options;
  }

  bool validate_option_(char s, const char *l) {
    return this->validate_option(s, l);
  }

  bool parse_short_option_(int argc, const char **argv, int &optind) {
    return this->parse_short_option(argc, argv, optind);
  }

  bool parse_long_option_(int argc, const char **argv, int &optind) {
    return this->parse_long_option(argc, argv, optind);
  }
};

TEST(OptionTests, AddOption) {
  ParserWrapper p;
  bool flag = false;
  std::vector<std::string> names;
  std::array<int, 2> range;

  EXPECT_TRUE(p.add_option(flag, "", 'f', "flag"));
  EXPECT_TRUE(p.add_option(names, "", 'n', ""));
  EXPECT_TRUE(p.add_option(range, "", 0, "range"));
  EXPECT_EQ(p.get_options().size(), 4);
}

TEST(OptionTests, AddOption_Duplicate) {
  ParserWrapper p;
  bool i1, i2, i3, i4;
  p.add_option(i1, "", '1', "flag1");
  p.add_option(i2, "", '2', "flag2");
  EXPECT_FALSE(p.add_option(i3, "", '1', ""));
  EXPECT_FALSE(p.add_option(i4, "", 0, "flag2"));
  EXPECT_EQ(p.get_options().size(), 3);
}

// Short option tests

TEST(OptionTests, ParseShortOption_Single) {
  ParserWrapper p;
  bool b = false;
  int i1 = 0;
  float f = 0.0f;
  int i2 = 0;
  bool b1=false, b2=false, b3=false;

  p.add_option(b, "", 'b', "");
  p.add_option(i1, "", 'i', "");
  p.add_option(f, "", 'f', "");
  p.add_option(i2, "", 'I', "");
  p.add_option(b1, "", '1', "");
  p.add_option(b2, "", '2', "");
  p.add_option(b3, "", '3', "");

  const char *argv[] = {"program_name", "-b", "-i", "10", "-f3.141", "-I", "-123"};
  const int argc = size(argv);
  int ind = 1;

  EXPECT_TRUE(p.parse_short_option_(argc, argv, ind));
  EXPECT_EQ(b, true);
  EXPECT_EQ(ind, 1);

  ind = 2;
  EXPECT_TRUE(p.parse_short_option_(argc, argv, ind));
  EXPECT_EQ(i1, 10);
  EXPECT_EQ(ind, 3);

  ind = 4;
  EXPECT_TRUE(p.parse_short_option_(argc, argv, ind));
  EXPECT_EQ(f, 3.141f);
  EXPECT_EQ(ind, 4);

  ind = 5;
  EXPECT_FALSE(p.parse_short_option_(argc, argv, ind));
  EXPECT_EQ(i2, 0);
  EXPECT_EQ(ind, 5);

  ind = 6;
  EXPECT_TRUE(p.parse_short_option_(argc, argv, ind));
  EXPECT_EQ(b1, true);
  EXPECT_EQ(b2, true);
  EXPECT_EQ(b3, true);
  EXPECT_EQ(ind, 6);
}

TEST(OptionTests, ParseShortOption_Multiple) {
  ParserWrapper p;
  std::array<int, 3> i;
  std::array<std::string, 2> s;

  p.add_option(i, "", 'i', "");
  p.add_option(s, "", 's', "");

  const char *argv[] = {"program_name", "-i", "1", "2", "3", "-s", "hello"};
  const int argc = size(argv);
  int ind = 1;

  EXPECT_TRUE(p.parse_short_option_(argc, argv, ind));
  EXPECT_EQ(i[0], 1);
  EXPECT_EQ(i[1], 2);
  EXPECT_EQ(i[2], 3);
  EXPECT_EQ(ind, 4);

  ind = 5;
  EXPECT_FALSE(p.parse_short_option_(argc, argv, ind));
  EXPECT_EQ(ind, 5);
}

TEST(OptionTests, ParseShortOption_Any) {
  ParserWrapper p;
  std::vector<int> i;

  p.add_option(i, "", 'i', "");

  const char *argv[] = {"program_name", "-i", "1", "-i", "2", "-i", "3"};
  const int argc = size(argv);
  int ind = 1;

  EXPECT_TRUE(p.parse_short_option_(argc, argv, ind));
  ind = 3;
  EXPECT_TRUE(p.parse_short_option_(argc, argv, ind));
  ind = 5;
  EXPECT_TRUE(p.parse_short_option_(argc, argv, ind));

  ASSERT_EQ(i.size(), 3);
  EXPECT_EQ(i[0], 1);
  EXPECT_EQ(i[1], 2);
  EXPECT_EQ(i[2], 3);
}

// Long option tests


TEST(OptionTests, ParseLongOption_Single) {
  ParserWrapper p;
  bool b = false;
  int i1 = 0;
  float f = 0.0f;
  int i2 = 0;
  int i3 = 0;

  p.add_option(b, "", 0, "bool");
  p.add_option(i1, "", 0, "int1");
  p.add_option(f, "", 0, "float");
  p.add_option(i2, "", 0, "int2");
  p.add_option(i3, "", 0, "int3");

  const char *argv[] = {"program_name", "--bool", "--int1", "10", "--float=3.141", "--int2", "--int3="};
  const int argc = size(argv);
  int ind = 1;

  EXPECT_TRUE(p.parse_long_option_(argc, argv, ind));
  EXPECT_EQ(b, true);
  EXPECT_EQ(ind, 1);

  ind = 2;
  EXPECT_TRUE(p.parse_long_option_(argc, argv, ind));
  EXPECT_EQ(i1, 10);
  EXPECT_EQ(ind, 3);

  ind = 4;
  EXPECT_TRUE(p.parse_long_option_(argc, argv, ind));
  EXPECT_EQ(f, 3.141f);
  EXPECT_EQ(ind, 4);

  ind = 5;
  EXPECT_FALSE(p.parse_long_option_(argc, argv, ind));
  EXPECT_EQ(ind, 5);

  ind = 6;
  EXPECT_FALSE(p.parse_long_option_(argc, argv, ind));
  EXPECT_EQ(ind, 6);
}

TEST(OptionTests, ParseLongOption_Multiple) {
  ParserWrapper p;
  std::array<int, 3> ints;
  std::array<std::string, 2> strings;
  std::array<float, 2> floats;

  p.add_option(ints, "", 0, "ints");
  p.add_option(strings, "", 0, "strings");

  const char *argv[] = {"program_name", "--ints", "1", "2", "3", "--strings", "hello", "--floats=1.0", "1.0"};
  const int argc = size(argv);
  int ind = 1;

  EXPECT_TRUE(p.parse_long_option_(argc, argv, ind));
  EXPECT_EQ(ints[0], 1);
  EXPECT_EQ(ints[1], 2);
  EXPECT_EQ(ints[2], 3);
  EXPECT_EQ(ind, 4);

  ind = 5;
  EXPECT_FALSE(p.parse_long_option_(argc, argv, ind));
  EXPECT_EQ(ind, 5);

  ind = 7;
  EXPECT_FALSE(p.parse_long_option_(argc, argv, ind));
  EXPECT_EQ(ind, 7);
}
