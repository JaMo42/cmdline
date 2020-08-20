/*
Copyright (c) 2020 Jakob Mohrbacher

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

#include <cstdio>
#include <cstring>

#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <sstream>

#include <algorithm>
#include <functional>

namespace cmdline {

struct Option {
  char short_name;
  std::string long_name;
  std::string help;
  std::string argument_name;

  bool takes_argument;
  size_t nargs;
  std::function<bool(const char **)> set_value;
};


struct Argument {
  std::string name;
  std::string help;
  bool required;

  size_t nargs;
  std::function<bool(const char **)> set_value;
};


class ArgumentParser {
protected:
  std::vector<Option> m_options;
  std::vector<Argument> m_arguments;
  bool m_show_help { false }; //< output for the help option
  std::stringstream m_ss; //< used for converting arguments

public:
  /**
   * Whether to show error messages.
   */
  bool error_messages = true;

  /**
   * Whether to allow abbreviations.
   *
   * If this is set to true, the argument `-fo' could match the option "foobar".
   * If this is set to true, short option can not be grouped anymore.
   */
  bool abbreviations = false;

  ArgumentParser();

  /**
   * @brief Adds a flag option.
   */
  bool add_option(bool &value, const char *help, char short_name, const char *long_name);
  /**
   * @brief Adds an option with one argument.
   */
  template<typename T>
  bool add_option(T &value, const char *help, char short_name, const char *long_name, const char *value_name = "");
  /**
   * @brief Adds an option with one argument, multiple occurrences will all be stored.
   */
  template<typename T>
  bool add_option(std::vector<T> &value, const char *help, char short_name, const char *long_name, const char *value_name = "");
  /**
   * @brief Adds an option with multiple arguments.
   */
  template<typename T, std::size_t N>
  bool add_option(std::array<T, N> &value, const char *help, char short_name, const char *long_name, const char *value_name = "");


  /**
   * @brief Adds an argument with one value.
   */
  template<typename T>
  void add_argument(T &value, const char *name, const char *help, bool required = true);
  /**
   * @brief Adds an argument with mutiple values
   */
  template<typename T, std::size_t N>
  void add_argument(std::array<T, N> &value, const char *name, const char *help, bool required = true);


  /**
   * @brief Parses arguments.
   */
  int parse_args(int argc, const char **argv, bool exit_on_failure = true);

  /**
   * @brief Prints the usage text.
   */
  void usage(const char *);

protected:
  using ArgumentsIt = std::vector<Argument>::iterator;
  static constexpr std::size_t npos = static_cast<std::size_t>(-1);

  bool validate_option(char short_name, const char *long_name);

  std::size_t option_index(char short_name);
  std::size_t option_index(const std::string_view long_name);

  bool parse_long_option(int, const char **, int &);
  bool parse_short_option(int, const char **, int &);
  bool parse_argument(int, const char **, int &, ArgumentsIt &);
};

///////////////////////////////////////////////////////////////////////////
// Implementations of template functions

template<typename T>
bool ArgumentParser::add_option(T &value, const char *help, char short_name, const char *long_name, const char *value_name) {
  if (!this->validate_option(short_name, long_name)) {
    return false;
  }
  m_options.emplace_back(
    short_name,
    long_name,
    help,
    value_name,
    true,
    1,
    [&](const char **arg) -> bool {
      m_ss.clear();
      m_ss.rdbuf()->str(*arg);
      m_ss >> value;
      return m_ss.rdbuf()->in_avail() == 0;
    }
  );
  return true;
}

template<typename T>
bool ArgumentParser::add_option(std::vector<T> &value, const char *help, char short_name, const char *long_name, const char *value_name) {
  if (!this->validate_option(short_name, long_name)) {
    return false;
  }
  m_options.emplace_back(
    short_name,
    long_name,
    help,
    value_name,
    true,
    1,
    [&](const char **arg) -> bool {
      T t;
      m_ss.clear();
      m_ss.rdbuf()->str(*arg);
      m_ss >> t;
      value.push_back(t);
      return m_ss.rdbuf()->in_avail() == 0;
    }
  );
  return true;
}

template<typename T, std::size_t N>
bool ArgumentParser::add_option(std::array<T, N> &value, const char *help, char short_name, const char *long_name, const char *value_name) {
  if (!this->validate_option(short_name, long_name)) {
    return false;
  }
  m_options.emplace_back(
    short_name,
    long_name,
    help,
    value_name,
    true,
    N,
    [&](const char **args) -> bool {
      for (unsigned int i = 0; i < N; ++i) {
        m_ss.clear();
        m_ss.rdbuf()->str(args[i]);
        m_ss >> value[i];
        if (m_ss.rdbuf()->in_avail() != 0)
          return false;
      }
      return true;
    }
  );
  return true;
}

template<typename T>
void ArgumentParser::add_argument(T &value, const char *name, const char *help, bool required) {
  m_arguments.emplace_back(
    name,
    help,
    required,
    1,
    [&](const char **arg) -> bool {
      m_ss.clear();
      m_ss.rdbuf()->str(*arg);
      m_ss >> value;
      return m_ss.rdbuf()->in_avail() == 0;
    }
  );
}

template<typename T, std::size_t N>
void ArgumentParser::add_argument(std::array<T, N> &value, const char *name, const char *help, bool required) {
  return m_arguments.emplace_back(
    name,
    help,
    required,
    N,
    [&](const char **args) -> bool {
      for (unsigned int i = 0; i < N; ++i) {
        m_ss.clear();
        m_ss.rdbuf()->str(args[i]);
        m_ss >> value[i];
        if (m_ss.rdbuf()->in_avail() != 0)
          return false;
      }
      return true;
    }
  );
}

}
