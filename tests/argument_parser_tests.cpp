#include "gtest/gtest.h"
#include "cmdline.h"

template<typename T, std::size_t N>
auto size(T (&)[N]) { return N; }

TEST(ArgumentParserTests, TerminateOptionParsing) {
  cmdline::ArgumentParser p;
  std::vector<const char *> unhandled;
  p.add_argument(unhandled);

  bool b1=false, b2=false, b3=false;
  int i = 0;
  std::string s = "";

  p.add_option(b1, "", '1', "");
  p.add_option(b2, "", '2', "");
  p.add_option(b3, "", '3', "");
  p.add_option(i, "", 'i', "");
  p.add_option(s, "", 's' ,"");

  const char *argv[] = {"program_name", "-1", "-i", "10", "--", "-12", "-s", "hello_world"};
  const int argc = size(argv);

  p.parse_args(argc, argv);

  ASSERT_EQ(unhandled.size(), 3);
  EXPECT_EQ(unhandled[0], argv[5]);
  EXPECT_EQ(unhandled[1], argv[6]);
  EXPECT_EQ(unhandled[2], argv[7]);
}

TEST(ArgumentParserTests, Abbreviations) {
  cmdline::ArgumentParser p;
  p.abbreviations = true;

  bool a=false, b=false, c=false;
  int aint = 0;
  int bint = 0;
  int binteger = 0;

  p.add_option(a, "", 'a', "");
  p.add_option(a, "", 'b', "");
  p.add_option(a, "", 'c', "");
  p.add_option(aint, "", 0, "aint");
  p.add_option(bint, "", 0, "bint");
  p.add_option(bint, "", 0, "binteger");

  const char *argv[] = {"program_name", "-a", "--a", "65", "-bi", "66", "-bc"};
  const int argc = size(argv);

  EXPECT_FALSE(p.parse_args(argc, argv, false));

  EXPECT_EQ(a, true);
  EXPECT_EQ(b, false);
  EXPECT_EQ(c, false);

  EXPECT_EQ(aint, 65);
  EXPECT_EQ(bint, 0);
  EXPECT_EQ(binteger, 0);
}

TEST(ArgumentParserTests, OptionalArguments) {
  cmdline::ArgumentParser p;

  int a=0, b=0, c=0;
  p.add_argument(a, "", "a");
  p.add_argument(b, "", "b", false);
  p.add_argument(c, "", "c", false);

  const char *argv[] = {"program_name", "1", "2"};
  const int argc = size(argv);

  EXPECT_TRUE(p.parse_args(argc, argv));
  EXPECT_EQ(a, 1);
  EXPECT_EQ(b, 2);
  EXPECT_EQ(c, 0);
}

TEST(ArgumentParserTests, MissingArguments) {
  cmdline::ArgumentParser p;

  int a=0, b=0;
  p.add_argument(a, "", "a");
  p.add_argument(b, "", "b");

  const char *argv[] = {"program_name", "1"};
  const int argc = size(argv);

  EXPECT_FALSE(p.parse_args(argc, argv, false));
  EXPECT_EQ(a, 1);
  EXPECT_EQ(b, 0);
}
