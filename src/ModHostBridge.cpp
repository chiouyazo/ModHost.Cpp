#include "headers/ModHostBridge.h"
#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <thread>

using json = nlohmann::json;

ModHostBridge::ModHostBridge(int port, const std::string& modName) : _modName(modName), socket_(_ioContext)
{

    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), port);

    std::cout << "Trying to connect to modhost..." << std::endl;

    while (true)
    {
        try
        {
            socket_.connect(endpoint);
            break;
        } catch (...)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }

    std::cout << "Connected to mod host." << std::endl;

    _listenerThread = std::thread(&ModHostBridge::ListenForResponses, this);
}

ModHostBridge::~ModHostBridge()
{
    socket_.close();
    if (_listenerThread.joinable())
    {
        _listenerThread.join();
    }
}

std::future<std::string> ModHostBridge::SendRequestAsync(
    const std::string& id, const std::string& platform,
    const std::string& handler, const std::string& eventType,
    const json& payload)
{

    std::promise<std::string> promise;
    auto future = promise.get_future();

    {
        std::lock_guard<std::mutex> lock(_pendingMutex);
        _pendingRequests[id] = std::move(promise);
    }

    json msg =
        {
        {"Id", id},
        {"Platform", platform},
        {"Handler", handler},
        {"Event", eventType},
        {"Payload", payload}
    };

    std::string serialized = msg.dump() + "\n";
    asio::write(socket_, asio::buffer(serialized));
    return future;
}

void ModHostBridge::SendResponse(const std::string& id, const std::string& platform,
                                 const std::string& handler, const std::string& eventType,
                                 const std::string& payload)
{
    json msg =
    {
        {"Id", id},
        {"Platform", platform},
        {"Handler", handler},
        {"Event", eventType},
        {"Payload", payload}
    };

    std::string serialized = msg.dump() + "\n";
    asio::write(socket_, asio::buffer(serialized));
}

void ModHostBridge::ListenForResponses()
{
    asio::streambuf buffer;
    try
    {
        while (true)
        {
            asio::read_until(socket_, buffer, "\n");
            std::istream is(&buffer);
            std::string line;
            std::getline(is, line);

            if (line.empty()) continue;

            json msg = json::parse(line, nullptr, false);
            if (msg.is_discarded())
            {
                std::cerr << "Invalid JSON: " << line << std::endl;
                continue;
            }

            std::string id = msg.value("Id", "");
            if (!id.empty())
            {
                std::lock_guard<std::mutex> lock(_pendingMutex);

                auto it = _pendingRequests.find(id);

                if (it != _pendingRequests.end())
                {
                    it->second.set_value(msg["Payload"].get<std::string>());
                    _pendingRequests.erase(it);
                    continue;
                }
            }

            HandleEvent(msg);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Listening thread error: " << e.what() << std::endl;
    }
}

void ModHostBridge::HandleEvent(const json& message)
{
    std::string platform = message.value("Platform", "");
    std::string handler = message.value("Handler", "");
    std::cout << "Received message:" + message << std::endl;

    if (platform == "CLIENT")
    {
        // TODO: clientBridge_->HandleEvent(message);
        return;
    }

    if (handler == "COMMAND")
    {
        // TODO: commandHandler_->HandleEvent(message);
    }
    else
    {
        std::cerr << "Unknown handler: " << handler << std::endl;
    }
}
