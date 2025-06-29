#ifndef CHATBOT_H
#define CHATBOT_H


#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include"../inc/Singleton.h"
using json = nlohmann::json;

class ChatBot : public Singleton<ChatBot> {

    friend class Singleton<ChatBot>;

public:
    
    ~ChatBot();
    std::string getChatResponse(const std::string& question, const std::string& model = "gpt-3.5-turbo");

private:
    ChatBot();
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
};

#endif
