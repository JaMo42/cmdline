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

#include <iostream>

namespace cmdline {
namespace detail {

std::string get_argument_name(char, const char *);

}

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
  std::vector<const char *> *m_unhandled { nullptr };
  std::string m_unhandled_name;

public:
  /**
   * Whether to show error messages.
   */
  bool error_messages = true;

  /**
   * Whether to allow abbreviations.
   *
   * If this is set to true, the argument `--fo' could match the option "foobar".
   * If this is set to true, short options can not be grouped anymore.
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
  bool add_option(T &value, const char *help, char short_name, const char *long_name, const char *argument_name = nullptr);
  /**
   * @brief Adds an option with one argument, multiple occurrences will all be stored.
   */
  template<typename T>
  bool add_option(std::vector<T> &value, const char *help, char short_name, const char *long_name, const char *argument_name = nullptr);
  /**
   * @brief Adds an option with multiple arguments.
   */
  template<typename T, std::size_t N>
  bool add_option(std::array<T, N> &value, const char *help, char short_name, const char *long_name, const char *argument_name = nullptr);


  /**
   * @brief Adds an argument with one value.
   */
  template<typename T>
  bool add_argument(T &value, const char *help, const char *name, bool required = true);
  /**
   * @brief Adds an argument with mutiple values
   */
  template<typename T, std::size_t N>
  bool add_argument(std::array<T, N> &value, const char *help, const char *name, bool required = true);
  /**
   * @brief Adds an argument that recieves all unhandled positional arguments.
   * This can only be called once, subsequent calls have no effect.
   */
  void add_argument(std::vector<const char *> &value, const char *name = "");


  /**
   * @brief Parses arguments.
   */
  bool parse_args(int argc, const char **argv, bool exit_on_failure = true);

  /**
   * @brief Prints the usage text.
   */
  void usage(FILE *, const char *);

protected:
  static constexpr std::size_t npos = static_cast<std::size_t>(-1);

  bool validate_option(char short_name, const char *long_name);

  std::size_t option_index(char short_name);
  std::size_t option_index(const std::string_view long_name);

  bool parse_long_option(int, const char **, int &);
  bool parse_short_option(int, const char **, int &);
  bool parse_argument(int, const char **, int &, std::size_t &);
};

///////////////////////////////////////////////////////////////////////////
// Implementations of template functions

template<typename T>
bool ArgumentParser::add_option(T &value, const char *help, char short_name, const char *long_name, const char *argument_name) {
  if (!this->validate_option(short_name, long_name)) {
    return false;
  }
  m_options.emplace_back(
    short_name,
    long_name,
    help,
    argument_name ? argument_name : std::move(detail::get_argument_name(short_name, long_name)),
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
bool ArgumentParser::add_option(std::vector<T> &value, const char *help, char short_name, const char *long_name, const char *argument_name) {
  if (!this->validate_option(short_name, long_name)) {
    return false;
  }
  m_options.emplace_back(
    short_name,
    long_name,
    help,
    argument_name ? argument_name : std::move(detail::get_argument_name(short_name, long_name)),
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
bool ArgumentParser::add_option(std::array<T, N> &value, const char *help, char short_name, const char *long_name, const char *argument_name) {
  if (!this->validate_option(short_name, long_name)) {
    return false;
  }
  m_options.emplace_back(
    short_name,
    long_name,
    help,
    argument_name ? argument_name : std::move(detail::get_argument_name(short_name, long_name)),
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
bool ArgumentParser::add_argument(T &value, const char *help, const char *name, bool required) {
  if (m_arguments.size() > 0 and required and !m_arguments.back().required) {
    std::fprintf(stderr, "required argument `%s' cannot follow optional arguments",
      name);
    return false;
  }
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
  return true;
}

template<typename T, std::size_t N>
bool ArgumentParser::add_argument(std::array<T, N> &value, const char *help, const char *name, bool required) {
  if (m_arguments.size() > 0 and required and !m_arguments.back().required) {
    std::fprintf(stderr, "required argument `%s' cannot follow optional arguments",
      name);
    return false;
  }
  m_arguments.emplace_back(
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
  return true;
}

}

