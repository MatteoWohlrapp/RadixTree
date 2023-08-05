#include "gtest/gtest.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <filesystem>

class MyEnvironment : public ::testing::Environment
{
public:
  void SetUp() override
  {
    try
    {
      spdlog::level::level_enum level = spdlog::level::debug;
      std::time_t time = std::time(0);
      std::tm *now = std::localtime(&time);
      std::ostringstream prefix;
      prefix << (now->tm_year + 1900) << '-'
             << (now->tm_mon + 1) << '-'
             << now->tm_mday << '-'
             << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << '_';

      auto path = "../logs/";
      // check if folder and files exist
      if (!std::filesystem::exists(path))
      {
        std::filesystem::create_directories(path);
      }

      std::string log_filename = path + prefix.str() + "log_tests.txt";
      auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_filename, false);
      auto logger = std::make_shared<spdlog::logger>("logger", sink);
      logger->set_level(level);
      spdlog::register_logger(logger);
    }
    catch (const spdlog::spdlog_ex &ex)
    {
      std::cout << "Log initialization failed: " << ex.what() << std::endl;
    }
  }
};

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new MyEnvironment);
  return RUN_ALL_TESTS();
}