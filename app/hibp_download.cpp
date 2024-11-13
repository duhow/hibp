#include "CLI/CLI.hpp"
#include "download/download.hpp"
#include "download/shared.hpp"
#include "hibp.hpp"
#include <cstdlib>
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/multi.h>
#include <event2/event.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <stdexcept>

void define_options(CLI::App& app, cli_config_t& cli_) {

  app.add_option("output_db_filename", cli_.output_db_filename,
                 "The file that the downloaded binary database will be written to")
      ->required();

  app.add_flag("--debug", cli_.debug,
               "Send verbose thread debug output to stderr. Turns off progress.");
  app.add_flag("--progress,!--no-progress", cli_.progress,
               "Show a progress meter on stderr. This is the default.");

  app.add_flag("--resume", cli_.resume,
               "Attempt to resume an earlier download. Not with --text-out.");

  app.add_flag("--text-out", cli_.text_out,
               "Output text format, rather than the default custom binary format.");

  app.add_flag("--force", cli_.force, "Overwrite any existing file!");

  app.add_option("--parallel-max", cli_.parallel_max,
                 "The maximum number of requests that will be started concurrently (default: 300)");

  app.add_option("--limit", cli_.prefix_limit,
                 "The maximum number (prefix) files that will be downloaded (default: 100 000 hex "
                 "or 1 048 576 dec)");
}

thread_logger logger; // NOLINT non-const-global
cli_config_t  cli;    // NOLINT non-const-global

int main(int argc, char* argv[]) {

  CLI::App app;
  define_options(app, cli);
  CLI11_PARSE(app, argc, argv);

  if (cli.debug) cli.progress = false;

  logger.debug = cli.debug;

  try {
    if (cli.text_out && cli.resume) {
      throw std::runtime_error("can't use `--resume` and `--text-out` together");
    }

    if (!cli.resume && !cli.force && std::filesystem::exists(cli.output_db_filename)) {
      throw std::runtime_error(std::format("File '{}' exists. Use `--force` to overwrite, or "
                                           "`--resume` to resume a previous download.",
                                           cli.output_db_filename));
    }

    auto mode = cli.text_out ? std::ios_base::out : std::ios_base::binary;

    if (cli.resume) {
      next_prefix = get_last_prefix(cli.output_db_filename) + 1;
      if (cli.prefix_limit <= next_prefix) {
        throw std::runtime_error(std::format("File '{}' contains {} records already, but you have "
                                             "specified --limit={}. Nothing to do. Aborting.",
                                             cli.output_db_filename, next_prefix,
                                             cli.prefix_limit));
      }
      start_prefix = next_prefix; // to make progress correct
      mode |= std::ios_base::app;
      std::cerr << std::format("Resuming from file {}\n", start_prefix);
    }

    auto output_db_stream = std::ofstream(cli.output_db_filename, mode);
    if (!output_db_stream) {
      throw std::runtime_error(std::format("Error opening '{}' for writing. Because: \"{}\".\n",
                                           cli.output_db_filename,
                                           std::strerror(errno))); // NOLINT errno
    }
    if (cli.text_out) {
      auto tw = text_writer(output_db_stream);
      run_downloads([&](const std::string& line) { tw.write(line); });

    } else {
      auto ffsw = flat_file::stream_writer<hibp::pawned_pw>(output_db_stream);
      run_downloads([&](const std::string& line) { ffsw.write(line); });
    }

  } catch (const std::exception& e) {
    std::cerr << std::format("Error: {}\n", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
