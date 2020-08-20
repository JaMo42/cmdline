#include "gtest/gtest.h"
#include "cmdline.h"

template<typename T, std::size_t N>
auto size(T (&)[N]) { return N; }

struct ParserWrapper : public cmdline::ArgumentParser {
  auto & get_arguments() {
    return m_arguments;
  }

  bool parse_argument_(int argc, const char **argv, int &optind, std::size_t &argind) {
    return this->parse_argument(argc, argv, optind, argind);
  }
};

TEST(ArgumentTests, ParseArgument_Single) {
  ParserWrapper p;
  int i = 0;
  float f = 0.0f;
  std::string s = "";

  p.add_argument(i, "", "int");
  p.add_argument(f, "", "float");
  p.add_argument(s, "", "string");

  const char *argv[] = {"program_name", "10", "3.141", "hello_world"};
  const int argc = size(argv);
  int ind = 1;
  std::size_t argind = 0;

  EXPECT_TRUE(p.parse_argument_(argc, argv, ind, argind));
  EXPECT_EQ(i, 10);

  ind = 2;
  EXPECT_TRUE(p.parse_argument_(argc, argv, ind, argind));
  EXPECT_EQ(f, 3.141f);

  ind = 3;
  EXPECT_TRUE(p.parse_argument_(argc, argv, ind, argind));
  EXPECT_EQ(s, "hello_world");
}

TEST(ArgumentTests, ParseArgument_Multiple) {
  ParserWrapper p;
  std::array<int, 3> ints;
  std::array<std::string, 2> strings;
  std::array<float, 2> floats;

  p.add_argument(ints, "", "ints");
  p.add_argument(strings, "", "strings");
  p.add_argument(floats, "", "floats");

  const char *argv[] = {"program_name", "1", "2", "3", "hello", "-", "1.0"};
  const int argc = size(argv);
  int ind = 1;
  std::size_t argind = 0;

  EXPECT_TRUE(p.parse_argument_(argc, argv, ind, argind));
  EXPECT_EQ(ints[0], 1);
  EXPECT_EQ(ints[1], 2);
  EXPECT_EQ(ints[2], 3);
  EXPECT_EQ(ind, 3);
  EXPECT_EQ(argind, 1);

  ind = 4;
  argind = 1;
  EXPECT_FALSE(p.parse_argument_(argc, argv, ind, argind));
  EXPECT_EQ(ind, 4);
  EXPECT_EQ(argind, 1);

  ind = 6;
  argind = 2;
  EXPECT_FALSE(p.parse_argument_(argc, argv, ind, argind));
  EXPECT_EQ(ind, 6);
  EXPECT_EQ(argind, 2);
}

TEST(ArgumentTests, Unhandled) {
  ParserWrapper p;
  int arg1, arg2, arg3;
  std::vector<const char *> unhandled;

  p.add_argument(arg1, "", "");
  p.add_argument(arg2, "", "");
  p.add_argument(arg3, "", "");

  {
    const char *argv[] = {"program_name", "1", "2", "3", "4"};
    const int argc = size(argv);
    int ind = 1;
    std::size_t argind = 0;

    EXPECT_TRUE(p.parse_argument_(argc, argv, ind, argind));
    EXPECT_EQ(argind, 1);

    ++ind;
    EXPECT_TRUE(p.parse_argument_(argc, argv, ind, argind));
    EXPECT_EQ(argind, 2);

    ++ind;
    EXPECT_TRUE(p.parse_argument_(argc, argv, ind, argind));
    EXPECT_EQ(argind, 3);

    ++ind;
    EXPECT_FALSE(p.parse_argument_(argc, argv, ind, argind));
    EXPECT_EQ(argind, 3);
  }

  p.add_argument(unhandled);

  {

    const char *argv[] = {"program_name", "1", "2", "3", "4", "5"};
    const int argc = size(argv);
    int ind = 1;
    std::size_t argind = 0;

    EXPECT_TRUE(p.parse_argument_(argc, argv, ind, argind));
    EXPECT_EQ(arg1, 1);
    EXPECT_EQ(argind, 1);

    ind = 2;
    EXPECT_TRUE(p.parse_argument_(argc, argv, ind, argind));
    EXPECT_EQ(arg2, 2);
    EXPECT_EQ(argind, 2);

    ind = 3;
    EXPECT_TRUE(p.parse_argument_(argc, argv, ind, argind));
    EXPECT_EQ(arg3, 3);
    EXPECT_EQ(argind, 3);

    ind = 4;
    EXPECT_TRUE(p.parse_argument_(argc, argv, ind, argind));
    ind = 5;
    EXPECT_TRUE(p.parse_argument_(argc, argv, ind, argind));
    ASSERT_EQ(unhandled.size(), 2);
    EXPECT_EQ(unhandled[0], argv[4]);
    EXPECT_EQ(unhandled[1], argv[5]);
  }

}
