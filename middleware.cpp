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

// Task structure
struct Task {
    std::string data;  // Task data
};

// Queue for tasks
std::queue<Task> taskQueue;
std::mutex queueMutex;

// Function to send HTTP POST request using libcurl
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void sendTaskToGPU(const std::string& url, const std::string& taskData) {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, taskData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    std::cout << "Response: " << readBuffer << std::endl;
}

// Read .gpus file and extract server URL
std::string readGPUAddress(const std::string& filepath) {
    std::ifstream file(filepath);
    std::string line;
    std::regex sshRegex("ssh (\\w+)@([\\w.-]+)");
    std::smatch matches;

    if (file.is_open()) {
        while (getline(file, line)) {
            if (std::regex_search(line, matches, sshRegex) && matches.size() > 2) {
                return "https://" + std::string(matches[2]) + ":8080/";
            }
        }
        file.close();
    }
    return "";
}

// Task processing function
void processTasks() {
    std::string gpuUrl = readGPUAddress("path_to_gpus_file.gpus");

    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (!taskQueue.empty()) {
            Task task = taskQueue.front();
            taskQueue.pop();
            lock.unlock();
            
            std::cout << "Sending task to GPU server: " << gpuUrl << std::endl;
            sendTaskToGPU(gpuUrl, task.data);

            std::this_thread::sleep_for(std::chrono::seconds(1));
        } else {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

int main() {
    std::thread processingThread(processTasks);

    crow::SimpleApp app;
    CROW_ROUTE(app, "/add_task").methods("POST"_method)([](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x)
            return crow::response(400, "Invalid JSON");

        Task newTask { x["data"].s() };
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            taskQueue.push(newTask);
        }

        return crow::response(200, "Task added successfully");
    });

    app.port(8080).multithreaded().run();

    processingThread.join();
    return 0;
}
