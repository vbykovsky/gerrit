#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/ini_parser.hpp"

using namespace std;

#define GERRIT_VERSION "v1.0.2"

const auto DIRECTORY_PATH = filesystem::temp_directory_path() / "gerrit";
const auto CONFIG_FFULL_NAME = DIRECTORY_PATH / "config.ini";
const string GERRIT_CONFIG = "GERRIT_CONFIG";

enum class CommandType : int {
  INIT,
  COMMIT,
  REVIEW,
  CONFIG_USER,
  CONFIG_REPO,
  VERSION,
  HELP
};

map<string, CommandType> commands = {
  { "init", CommandType::INIT },

  { "commit", CommandType::COMMIT },
  //alias
  { "c", CommandType::COMMIT },

  { "review", CommandType::REVIEW },
  //alias
  { "r", CommandType::REVIEW },

  { "config.user", CommandType::CONFIG_USER },
  //alias
  { "c.user", CommandType::CONFIG_USER },

  { "config.repo", CommandType::CONFIG_REPO },
  //alias
  { "c.repo", CommandType::CONFIG_REPO },

  { "-v", CommandType::VERSION },

  { "-help", CommandType::HELP },
  //alias
  { "-h", CommandType::HELP },
};

string getAvailableCommands(const map<string, CommandType> commands) {
  string result = "";

  for (const auto commandObj : commands) {
    result += commandObj.first;
    result += ", ";
  }

  result.pop_back();
  result.pop_back();

  return result;
}

bool isQuotes(const char& s) {
  return s == '"' || s == '\'';
}

void cmd(string command) {
  cout << command << endl;
  system(command.c_str());
}

void cmd(vector<string> commands) {
  for (const auto command : commands) {
    cmd(command);
  }
}

int main(int argc, char** arguments) {
  try {
    /*
    cout << "arguments(" << argc << "): ";
    for (int i = 0; i < argc; i++) {
      cout << arguments[i] << " ";
    }
    cout << endl;
    */

#pragma region Config file
    if (!filesystem::exists(DIRECTORY_PATH)) {
      std::filesystem::create_directories(DIRECTORY_PATH);
    }

    if (!filesystem::exists(CONFIG_FFULL_NAME)) {
      ofstream configFile(CONFIG_FFULL_NAME.string());
      configFile.close();
    }

    boost::property_tree::ptree configTree;
    boost::property_tree::ini_parser::read_ini(CONFIG_FFULL_NAME.string(), configTree);
#pragma endregion

#pragma region Get command
    string fullFileName = arguments[0];
    argc -= 1;

    if (argc < 1) {
      throw runtime_error("Invalid number of args(min: 1)");
    }

    string command = arguments[1];
    if (commands.find(command) == commands.end()) {
      throw runtime_error("Invalid command(available: " + getAvailableCommands(commands) + ")");
    }

    auto commandType = commands[command];
#pragma endregion

    //CONFIG.USER
    if (commandType == CommandType::CONFIG_USER) {
      if (argc < 2) {
        throw runtime_error("Invalid number of args(min: 1)");
      }

      configTree.put(GERRIT_CONFIG + ".user", arguments[2]);

      boost::property_tree::ini_parser::write_ini(CONFIG_FFULL_NAME.string(), configTree);
    }
    //CONFIG.REPO
    else if (commandType == CommandType::CONFIG_REPO) {
      if (argc < 2) {
        throw runtime_error("Invalid number of args(min: 1)");
      }

      configTree.put(GERRIT_CONFIG + ".repo", arguments[2]);

      boost::property_tree::ini_parser::write_ini(CONFIG_FFULL_NAME.string(), configTree);
    }
    //INIT
    else if (commandType == CommandType::INIT) {
      string user = "";
      string repo = "";
      auto _user = configTree.get_optional<string>(GERRIT_CONFIG + ".user");
      auto _repo = configTree.get_optional<string>(GERRIT_CONFIG + ".repo");

      if (argc == 1) {
        if (!_user.has_value() || !_repo.has_value()) {
          throw runtime_error("Invalid number of args(min: 1)");
        }

        user = _user.get();
        repo = _repo.get();
      }
      else if (argc == 2) {
        if (!_user.has_value()) {
          throw runtime_error("Invalid number of args(min: 2)");
        }

        user = _user.get();
        repo = arguments[2];
      }
      else if (argc == 3) {
        user = arguments[2];
        repo = arguments[3];
      }

      cmd({
        string("git clone \"https://" + user + "@gerrit.delivery.epam.com/a/" + repo + "\""),
        string("cd \"" + repo + "\""),
        string("mkdir - p.git / hooks"),
        string("curl - Lo `git rev - parse --git - dir`/hooks / commit - msg https ://" + user + "@gerrit.delivery.epam.com/tools/hooks/commit-msg; chmod +x `git rev-parse --git-dir`/hooks/commit-msg)"),
        string("git add origin gerrit \"https://" + user + "@gerrit.delivery.epam.com/a/" + repo + "")
      });
    }
    //COMMIT
    else if (commandType == CommandType::COMMIT) {
      if (argc < 2) {
        throw runtime_error("Invalid number of args(min: 1)");
      }

      string comment = arguments[2];
      comment.erase(remove_if(comment.begin(), comment.end(), isQuotes));

      cmd(string("git commit -m \"" + comment + "\"").c_str());
    }
    //REVIEW
    else if (commandType == CommandType::REVIEW) {
      string branch = "";

      if (argc == 1) {
        branch = "master";
      }
      else {
        branch = arguments[2];
      }

      branch.erase(remove_if(branch.begin(), branch.end(), isQuotes));

      cmd(string("git push gerrit HEAD:refs/for/"+ branch).c_str());
    }
    //GERRIT_VERSION
    else if (commandType == CommandType::VERSION) {
      cout << "Gerrit version: " << GERRIT_VERSION << endl;
    }
    //HELP
    else if (commandType == CommandType::HELP) {
      cout << "Gerrit. Available commands: " << endl;
      cout << "    - init: init repo for user. Variants of use: 1) init <user> <repo>. 2) init <repo>. 3) init. Missing parameters are taken from the config." << endl;
      cout << "    - commit(alias: c) <message>: git commit -m \"<message>\"" << endl;
      cout << "    - review(alias: r) <branch>: git push gerrit HEAD:refs/for/<branch>" << endl;
      cout << "    - config.user(alias: c.user) <user name>: save default user to config" << endl;
      cout << "    - config.repo(alias: c.repo) <repo>: save default repo to config" << endl;
      cout << "    - -help(alias: -h): help" << endl;
      cout << "    - -v: version" << endl;
    }
    else {
      throw runtime_error("Invalid command(available: " + getAvailableCommands(commands) + ")");
    }
  } catch (const exception& error) {
    cout << error.what() << endl;
  }

  return 0;
}