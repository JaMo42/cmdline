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
#include "cmdline.h"

namespace cmdline {
namespace detail {

std::string get_argument_name(char short_name, const char *long_name) {
  if (long_name != nullptr) {
    std::string s(long_name);
    std::for_each(s.begin(), s.end(), [](char &ch) { ch = ch=='-' ? '_' : ::toupper(ch); });
    return s;
  }
  else {
    return std::string(1, ::toupper(short_name));
  }
}

}

ArgumentParser::ArgumentParser() {
  this->add_option(m_show_help, "Display this message", 0, "help");
}

bool ArgumentParser::add_option(bool &value, const char *help, char short_name, const char *long_name) {
  if (!this->validate_option(short_name, long_name)) {
    return false;
  }
  m_options.emplace_back(
    short_name,
    long_name,
    help,
    "",
    false,
    0,
    [&value](const char **) -> bool {
      value = true;
      return true;
    }
  );
  return true;
}

void ArgumentParser::add_argument(std::vector<const char *> &value, const char *name) {
  if (m_unhandled == nullptr) {
    m_unhandled = &value;
    m_unhandled_name = name;
  }
}

bool ArgumentParser::validate_option(char short_name, const char *long_name) {
  bool cs = short_name != 0;
  bool cl = long_name[0] != '\0';
  // Check if either of the names already exists
  for (auto &o : m_options) {
    if (cs and o.short_name == short_name) {
      std::fprintf(stderr, "duplicate option -- %c\n", short_name);
      return false;
    }
    if (cl and o.long_name == long_name) {
      std::fprintf(stderr, "duplicate option `%s'\n", long_name);
      return false;
    }
  }
  return true;
}

bool ArgumentParser::parse_args(int argc, const char **argv, bool exit_on_failure) {
  std::size_t argind = 0;
  bool terminate_options = false;

  auto print_usage_and_exit = [&](int code) {
    this->usage(stderr, argv[0]);
    if (exit_on_failure) {
      std::exit(code);
    }
  };

  for (int i = 1; i < argc; ++i) {
    if (!terminate_options and argv[i][0] == '-') {
      if (argv[i][1] == '-'
          or (abbreviations and (argv[i][2] or this->option_index(argv[i][1]) == npos))) {
        if (!strcmp(argv[i], "--")) {
          terminate_options = true;
          continue;
        }
        if (!this->parse_long_option(argc, argv, i)) {
          print_usage_and_exit(1);
          return false;
        }
      }
      else {
        if (!this->parse_short_option(argc, argv, i)) {
          print_usage_and_exit(1);
          return false;
        }
      }
    }
    else {
      if (!this->parse_argument(argc, argv, i, argind)) {
        print_usage_and_exit(1);
        return false;
      }
    }
  }

  // Check if all required arguments where handled
  if (argind != m_arguments.size() and m_arguments[argind].required) {
    for (std::size_t i = argind; i < m_arguments.size() and m_arguments[i].required; ++i) {
      std::fprintf(stderr, "%s: argument `%s' is required\n",
        argv[0], m_arguments[i].name.c_str());
    }
    print_usage_and_exit(1);
    return false;
  }

  if (m_show_help) {
    print_usage_and_exit(0);
    return false;
  }

  return true;
}

std::size_t ArgumentParser::option_index(char short_name) {
  for (std::size_t i = 0; i < m_options.size(); ++i) {
    if (m_options[i].short_name == short_name) {
      return i;
    }
  }
  return npos;
}

std::size_t ArgumentParser::option_index(const std::string_view long_name) {
  for (std::size_t i = 0; i< m_options.size(); ++i) {
      if (m_options[i].long_name == long_name) {
        return i;
      }
    }
  return npos;
}

bool ArgumentParser::parse_long_option(int argc, const char **argv, int &optind) {
  const std::string_view tok(argv[optind]);
  const std::size_t eq_pos = tok.find('=');
  const int off = 1 + static_cast<int>(argv[optind][1] == '-');
  const std::string_view name = tok.substr(off, eq_pos - off);
  std::size_t index_found = npos;
  bool ok = true;
  std::size_t i;

  if (!abbreviations) {
    index_found = this->option_index(name);
  }
  else {
    bool ambiguous = false;
    for (i = 0; i < m_options.size(); ++i) {
      if (!strncmp(name.data(), m_options[i].long_name.c_str(), name.length())) {
        if (m_options[i].long_name.length() == name.length()) {
          // exact match found
          index_found = i;
          ambiguous = false;
          break;
        }
        else if (index_found == npos) {
          // first non-exact match
          index_found = i;
        }
        else {
          ambiguous = true;
        }
      }
    }
    if (ambiguous) {
      if (error_messages) {
        std::fprintf(stderr, "%s: option `%s' is ambiguous\n",
          argv[0], argv[optind]);
      }
      return false;
    }
  }

  if (index_found == npos) {
    if (error_messages) {
      std::fprintf(stderr, "%s: unrecognized option `--%.*s'\n",
        argv[0], static_cast<int>(name.length()), name.data());
    }
    return false;
  }

  Option &opt = m_options[index_found];

  auto print_arg_error = [&]() {
    if (opt.nargs == 1) {
      std::fprintf(stderr, "%s: option `%s' requires an argument\n",
        argv[0], argv[optind]);
    }
    else {
      std::fprintf(stderr, "%s: option `%s' requires %zu arguments\n",
        argv[0], argv[optind], opt.nargs);
    }
  };

  if (opt.takes_argument) {
    if (eq_pos == std::string::npos) {
      // Check if there are enough argv elements left
      if ((optind + opt.nargs) >= static_cast<std::size_t>(argc)) {
        if (error_messages) {
          print_arg_error();
        }
        return false;
      }
      // Check if there are enough arguments
      for (std::size_t n = 0; n < opt.nargs; ++n) {
        if (argv[optind + n + 1][0] == '-') {
          if (error_messages) {
            print_arg_error();
          }
          return false;
        }
      }
      // All good
      ok = opt.set_value(&argv[optind + 1]);
      optind += opt.nargs;
    }
    else {
      if (opt.nargs > 1) {
        if (error_messages) {
          std::fprintf(stderr, "%s: option `--%.*s' requires %zu arguments\n",
            argv[0], static_cast<int>(name.length()), name.data(), opt.nargs);
        }
        return false;
      }
      const char *arg = argv[optind] + eq_pos + 1;
      if (*arg == '\0') {
        if (error_messages) {
          std::fprintf(stderr, "%s: option `--%.*s' requires an argument\n",
            argv[0], static_cast<int>(name.length()), name.data());
        }
        return false;
      }
      ok = opt.set_value(&arg);
    }
  }
  else {
    opt.set_value(nullptr);
  }

  return ok;
}

bool ArgumentParser::parse_short_option(int argc, const char **argv, int &optind) {
  bool ok = true;
  std::size_t index = this->option_index(argv[optind][1]);

  if (index == npos) {
    if (error_messages) {
      std::fprintf(stderr, "%s: invalid option -- %c\n",
        argv[0], argv[optind][1]);
    }
    return false;
  }

  Option &opt = m_options[index];

  auto print_arg_error = [&]() {
    if (opt.nargs == 1) {
      std::fprintf(stderr, "%s: option requires an argument -- %c\n",
        argv[0], argv[optind][1]);
    }
    else {
      std::fprintf(stderr, "%s: option requires %zu arguments -- %c\n",
        argv[0], opt.nargs, argv[optind][1]);
    }
  };

  if (opt.takes_argument) {
    if (argv[optind][2]) {
      // There's something else in the argv element, assume it's the argument
      if (opt.nargs > 1) {
        if (error_messages) {
          std::fprintf(stderr, "%s: option requires %zu arguments -- %c\n",
            argv[0], opt.nargs, argv[optind][1]);
        }
        return false;
      }
      const char *arg = argv[optind] + 2;
      ok = opt.set_value(&arg);
    }
    else {
      // Check if there are enough argv elements left
      if ((optind + opt.nargs) >= static_cast<std::size_t>(argc)) {
        if (error_messages) {
          print_arg_error();
        }
        return false;
      }
      // Check if there are enough arguments
      for (std::size_t n = 0; n < opt.nargs; ++n) {
        if (argv[optind + n + 1][0] == '-') {
          if (error_messages) {
            print_arg_error();
          }
          return false;
        }
      }
      // All good
      ok = opt.set_value(&argv[optind + 1]);
      optind += opt.nargs;
    }
  }
  else {
    opt.set_value(nullptr);
    for (std::size_t i = 2; i < std::strlen(argv[optind]); ++i) {
      index = this->option_index(argv[optind][i]);
      if (index == npos) {
        if (error_messages) {
          std::fprintf(stderr, "%s: invalid option -- %c\n",
            argv[0], argv[optind][1]);
        }
        return false;
      }
      if (m_options[index].takes_argument) {
        // Options taking an argument can not be grouped
        if (error_messages) {
          std::fprintf(stderr, "%s: option requires an argument -- %c\n",
            argv[0], argv[optind][i]);
        }
        return false;
      }
      m_options[index].set_value(nullptr);
    }
  }

  return ok;
}

bool ArgumentParser::parse_argument(int argc, const char **argv, int &optind, std::size_t &argind) {
  if (argind >= m_arguments.size()) {
    if (m_unhandled != nullptr) {
      m_unhandled->push_back(argv[optind]);
      return true;
    }
    else {
      std::fprintf(stderr, "%s: unrecognized argument: `%s'\n",
        argv[0], argv[optind]);
      return false;
    }
  }

  Argument &arg = m_arguments[argind];

  auto print_arg_error = [&]() {
    if (arg.nargs == 1) {
      std::fprintf(stderr, "%s: argument `%s' requires an argument\n",
        argv[0], arg.name.c_str());
    }
    else {
      std::fprintf(stderr, "%s: argument `%s' requires %zu arguments\n",
        argv[0], arg.name.c_str(), arg.nargs);
    }
  };

  // Check if there are enough argv elements left
  if ((optind + arg.nargs - 1) >= static_cast<std::size_t>(argc)) {
    if (error_messages) {
      print_arg_error();
    }
    return false;
  }
  // Check if there are enough arguments
  for (std::size_t n = 0; n < arg.nargs; ++n) {
    if (argv[optind + n][0] == '-') {
      if (error_messages) {
        print_arg_error();
      }
      return false;
    }
  }
  // All good
  arg.set_value(&argv[optind]);
  optind += arg.nargs - 1;
  ++argind;
  return true;
}

namespace detail {
void print(FILE *f, char ch) {
  std::fputc(ch, f);
}
void print(FILE *f, const char *str) {
  std::fputs(str, f);
}
template<typename... Args>
void print(FILE *f, const char *fmt, const Args&... args) {
  std::fprintf(f, fmt, args...);
}
}

void ArgumentParser::usage(FILE *file, const char *program_name) {
  auto print = [&file]<typename... Args>(const Args&... args) {
    detail::print(file, args...);
  };
  auto print_opt_name = [&](const Option &o) {
    if (o.short_name) {
      print("-%c", o.short_name);
    }
    else {
      print("--%s", o.long_name.c_str());
    }
  };

  // Width of the option and argument names column
  constexpr int NAMES_WIDTH = 24;

  print("Usage: %s", program_name);

  for (Option &opt : m_options) {
    print(" [");
    print_opt_name(opt);
    if (opt.nargs > 0) {
      for (std::size_t n = 0; n < opt.nargs; ++n) {
        print(" %s", opt.argument_name.c_str());
      }
    }
    print(']');
  }

  for (Argument &arg : m_arguments) {
    print(' ');
    if (not arg.required) {
      print('[');
    }
    print(arg.name.c_str());
    for (std::size_t n = 1; n < arg.nargs; ++n) {
      print(" %s", arg.name.c_str());
    }
    if (not arg.required) {
      print(']');
    }
  }
  if (m_unhandled) {
    print(' ');
    if (not m_unhandled_name.empty()) {
      print(m_unhandled_name.c_str());
    }
    print("...");
  }
  print('\n');

  print("\nOptions:\n");
  int written;
  for (Option &opt : m_options) {
    written = 2;
    print("  ");
    if (opt.short_name != 0) {
      print("-%c", opt.short_name);
      written += 2;
      if (not opt.long_name.empty()) {
        print(", ");
        written += 2;
      }
    }
    if (not opt.long_name.empty()) {
      print("--%s", opt.long_name.c_str());
      written += 2 + opt.long_name.length();
    }

    if (opt.nargs > 0) {
      print(" ");
      for (std::size_t n = 0; n < opt.nargs; ++n) {
        print(" %s", opt.argument_name.c_str());
      }
      written += 1 + (1 + opt.argument_name.length()) * opt.nargs;
    }

    if (written >= NAMES_WIDTH) {
      print('\n');
      written = 0;
    }

    for (int i = written; i < NAMES_WIDTH; ++i) {
      print(' ');
    }
    print(opt.help.c_str());
    print('\n');
  }

  print("\nArguments:\n");
  for (Argument &arg : m_arguments) {
    print("  %s", arg.name.c_str());
    if (not arg.help.empty()) {
      for (int i = 2+arg.name.length(); i < NAMES_WIDTH; ++i) {
        print(' ');
      }
      print(arg.help.c_str());
    }
    print('\n');
  }
}

}

