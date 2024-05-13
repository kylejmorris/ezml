#include "crow_all.h"
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <regex>
#include <sstream>
#include <string>
#include <thread>

namespace fs = std::filesystem;

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}


void init_project(const std::string& project_name) {
    fs::create_directory(project_name);
    std::ofstream weights_file(project_name + "/weights.pl");
    weights_file << "Default BERT model weights";
    weights_file.close();

    std::ofstream gpus_file(project_name + "/.gpus");
    gpus_file << "List of GPU SSH keys";
    gpus_file.close();

    fs::create_directory(project_name + "/server");
}

#include <iostream>
#include <fstream>
#include <cstdlib>  // For system()

void deploy_project() {
    std::ifstream file("my_project/.gpus");
    std::string ssh_command;

    if (!file.is_open()) {
        std::cerr << "Failed to open .gpus file" << std::endl;
        return;
    }

    while (getline(file, ssh_command)) {
        if (ssh_command.empty()) continue;

        // Ensure server directory exists
        std::string create_dir_command = ssh_command + " 'mkdir -p ~/server'";
        system(create_dir_command.c_str());

        // Copy server directory
        std::cout << "Copying server directory to " << ssh_command.substr(4) << std::endl;
        std::string scp_command = "scp -i ~/.ssh/id_ed25519 -r ./my_project/server/* " + ssh_command.substr(0, ssh_command.find(" -i")) + ":~/server/";
        system(scp_command.c_str());

        // Run the server
        std::string run_command = ssh_command + " 'cd ~/server && ./run_server > /dev/null 2>&1 &'";
        system(run_command.c_str());

        std::cout << "Server deployed and started on: " << ssh_command.substr(4) << std::endl;
    }

    file.close();
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string command = argv[1];
        if (command == "init" && argc == 3) {
            init_project(argv[2]);
        } else if (command == "deploy") {
            deploy_project();
        } else {
            std::cout << "Invalid command" << std::endl;
        }
    } else {
        std::cout << "No command provided" << std::endl;
    }
    return 0;
}
