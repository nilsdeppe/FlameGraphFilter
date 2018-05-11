/*!
@file
@copyright Nils Deppe 2018
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
*/

#include <algorithm>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <regex>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace po = boost::program_options;

/*!
 * \brief Stream operator for `std::vector` is require by Boost.ProgramOptions
 */
template <class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
  os << "(";
  std::copy(v.begin(), std::prev(v.end()), std::ostream_iterator<T>(os, ", "));
  os << v.back() << ")";
  return os;
}

/*!
 * \brief Returns the lowest stack frame. Specifically, if the sample is
 * collected in: `main()->foo()->bar()->baz()` it will return `baz`
 */
std::string get_lowest_stack(const std::string& full_stack_and_sample_count) {
  const auto location_of_last_semicolon =
      full_stack_and_sample_count.find_last_of(';') + 1;
  return full_stack_and_sample_count.substr(
      location_of_last_semicolon,
      full_stack_and_sample_count.find_last_of(' ') -
          location_of_last_semicolon);
}

/*!
 * \brief Returns the number of samples collected for the specific stack trace
 */
size_t get_sample_count(const std::string& full_stack_and_sample_count) {
  return std::atoi(full_stack_and_sample_count
                       .substr(full_stack_and_sample_count.find_last_of(' '))
                       .c_str());
}

/*!
 * \brief Builds a map between the lowest stack frame and a pair of the total
 * samples of that lowest stack frame and a vector of the stack trace
 */
std::map<std::string, std::tuple<size_t, std::vector<std::string>>>
build_stack_map(const std::string& filename) {
  std::ifstream folded_file{filename};
  if (not folded_file.is_open()) {
    std::cerr << "Could not open file: " << filename << " for reading\n";
    std::exit(1);
  }
  std::string line;
  std::map<std::string, std::tuple<size_t, std::vector<std::string>>>
      stack_map{};
  while (std::getline(folded_file, line)) {
    const std::string lowest_stack = get_lowest_stack(line);
    if (stack_map.find(lowest_stack) != stack_map.end()) {
      auto& current_stack = stack_map[lowest_stack];
      std::get<0>(current_stack) += get_sample_count(line);
      std::get<1>(current_stack).push_back(line);
    } else {
      stack_map[lowest_stack] = std::tuple<size_t, std::vector<std::string>>{
          get_sample_count(line), std::vector<std::string>{line}};
    }
  }
  folded_file.close();
  return stack_map;
}

/*!
 * \brief From the full map returns only the stack traces that have a percentage
 * of the total samples greater than the cutoff percentage and are in the list
 * of functions to show (also set by user input). If the list of functions to
 * show is empty then all functions that have a sample percentage about the
 * cutoff percentage are show.
 */
std::vector<std::tuple<size_t, std::vector<std::string>>> filter_stack(
    std::map<std::string, std::tuple<size_t, std::vector<std::string>>>
        stack_map,
    const double cutoff_percentage,
    const std::vector<std::string>& regexes_to_show) {
  const size_t total_samples = std::accumulate(
      stack_map.begin(), stack_map.end(), size_t{0},
      [](const size_t state,
         const std::pair<const std::string,
                         std::tuple<size_t, std::vector<std::string>>>&
             element) { return state + std::get<0>(element.second); });
  using IteratorType = typename std::decay<decltype(stack_map)>::type::iterator;
  std::vector<std::tuple<size_t, std::vector<std::string>>> filtered_stacks{};
  std::for_each(
      std::move_iterator<IteratorType>(stack_map.begin()),
      std::move_iterator<IteratorType>(stack_map.end()),
      [&filtered_stacks, &total_samples, &cutoff_percentage, &regexes_to_show](
          typename std::decay<decltype(stack_map)>::type::value_type&& t) {
        if (static_cast<double>(std::get<0>(t.second)) /
                static_cast<double>(total_samples) >
            0.01 * cutoff_percentage) {
          if (regexes_to_show.empty()) {
            filtered_stacks.push_back(std::move(t.second));
          } else {
            const auto stack_frame = get_lowest_stack(std::get<1>(t.second)[0]);
            for (const auto& regex_string : regexes_to_show) {
              std::regex expression(regex_string);
              if (std::regex_match(stack_frame.begin(), stack_frame.end(),
                                   expression)) {
                filtered_stacks.push_back(std::move(t.second));
                break;
              }
            }
          }
        }
      });
  return filtered_stacks;
}

//
/*!
 * \brief Removes the top of the stack. That is, for main()->foo()->bar()->baz()
 * with a limit of two main() and foo() would be removed. the bottom of the
 * stack. For
 */
std::vector<std::tuple<size_t, std::vector<std::string>>> shrink_to_stack_limit(
    std::vector<std::tuple<size_t, std::vector<std::string>>> stacks_map,
    const size_t stack_limit) {
  if (stack_limit == 0) {
    return stacks_map;
  }
  for (auto& stack_list : stacks_map) {
    for (auto& stack : std::get<1>(stack_list)) {
      // We remove the unwanted stacks by recursive calls to find
      typename std::string::size_type current_position = stack.size();
      for (size_t i = 0;
           i < stack_limit and current_position != std::string::npos; ++i) {
        current_position = stack.rfind(';', current_position - 1);
      }
      if (current_position != std::string::npos) {
        stack = stack.substr(current_position + 1, std::string::npos);
      }
    }
  }
  return stacks_map;
}

/*!
 * \brief Write the stack list return by `shrink_to_stack_limit` to disk
 */
void write_filtered_stack_to_file(
    const std::vector<std::tuple<size_t, std::vector<std::string>>>& stacks,
    const std::string& out_filename) {
  std::ofstream out_file(out_filename);
  if (not out_file.is_open()) {
    std::cerr << "Could not open file: " << out_filename << " for writing\n";
    std::exit(1);
  }
  for (const auto& stack_list : stacks) {
    for (const auto& stack : std::get<1>(stack_list)) {
      out_file << stack << '\n';
    }
  }
  out_file.close();
}

int main(int argc, char* argv[]) {
  try {
    po::options_description options_description("Allowed options");
    options_description.add_options()         //
        ("help", "Print this help message.")  //
        ("cutoff-percentage", po::value<double>()->default_value(0.5),
         "Function calls that take up less than cutoff-percentage of the total "
         "runtime are not displayed.")  //
        ("stack-limit", po::value<size_t>()->default_value(0),
         "If set to a value greater than zero then the displayed inverse stack "
         "depth is limited to stack-limit frames. That is, for "
         "main()->foo()->bar()->baz() and a limit of 2 main() and foo() are "
         "removed.")  //
        ("show", po::value<std::vector<std::string>>()->composing(),
         "A list of regular expressions (run through the C++ STL regex "
         "library) to be shown. If none are specified then everything is "
         "shown.")  //
        ("output,o", po::value<std::string>(),
         "The name of the output file.")  //
        ("input-file", po::value<std::string>(), "The name of the input file.");

    po::positional_options_description input_file_opt;
    input_file_opt.add("input-file", -1);

    po::variables_map args;
    po::store(po::command_line_parser(argc, argv)
                  .options(options_description)
                  .positional(input_file_opt)
                  .run(),
              args);
    po::notify(args);

    if (args.count("help")) {
      std::cout << options_description << "\n";
      return 0;
    }

    if (not args.count("output")) {
      std::cerr << "You must set the output file.\n"
                << options_description << "\n";
      std::exit(1);
    }
    if (not args.count("input-file")) {
      std::cerr << "Must specify an input file.\n"
                << options_description << "\n";
    }
    std::vector<std::string> regexes_to_show{};
    if (args.count("show")) {
      regexes_to_show = args["show"].as<std::vector<std::string>>();
    }

    write_filtered_stack_to_file(
        shrink_to_stack_limit(
            filter_stack(build_stack_map(args["input-file"].as<std::string>()),
                         args["cutoff-percentage"].as<double>(),
                         regexes_to_show),
            args["stack-limit"].as<size_t>()),
        args["output"].as<std::string>());

  } catch (std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  } catch (...) {
    std::cerr << "Exception of unknown type!\n";
  }
  return 0;
}
