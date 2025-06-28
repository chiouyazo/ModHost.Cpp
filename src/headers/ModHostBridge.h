#pragma once

#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <thread>
#include <future>
#include <mutex>
#include <string>

class CommandHandler;
class ItemHandler;
class BlockHandler;
class ClientBridge;

class ModHostBridge
{
public:
    ModHostBridge(int port, const std::string& modName);
    ~ModHostBridge();

    std::future<std::string> SendRequestAsync(const std::string& id, const std::string& platform,
                                              const std::string& handler, const std::string& eventType,
                                              const nlohmann::json& payload);

    void SendResponse(const std::string& id, const std::string& platform,
                      const std::string& handler, const std::string& eventType,
                      const std::string& payload);

private:
    void ListenForResponses();
    void HandleEvent(const nlohmann::json& message);

    std::string _modName;
    asio::io_context _ioContext;
    asio::ip::tcp::socket socket_;
    std::thread _listenerThread;

    std::unordered_map<std::string, std::promise<std::string>> _pendingRequests;
    std::mutex _pendingMutex;
    //
    // std::unique_ptr<ClientBridge> clientBridge_;
    // std::unique_ptr<CommandHandler> commandHandler_;
    // std::unique_ptr<ItemHandler> itemHandler_;
    // std::unique_ptr<BlockHandler> blockHandler_;
};