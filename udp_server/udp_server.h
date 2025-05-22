//udp_server.h
#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <boost/asio.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <set>
#include <mutex>
#include <atomic>
#include <thread>
#include <QDebug>

using boost::asio::ip::udp;

class UDPServer {
public:
    explicit UDPServer(unsigned short port);
    ~UDPServer();

    void start();
    void stop();

    bool isRunning() const;
    void registerClient(const udp::endpoint& client_endpoint);
    void unregisterClient(const udp::endpoint& client_endpoint);
private:
    void run(); // внутренний цикл работы сервера
    void start_receive();

    void handle_receive(const boost::system::error_code& error, std::size_t length);
    void sendToClients(const char* data, std::size_t length, const udp::endpoint& exclude_endpoint);

    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
    unsigned short port_;
    udp::socket socket_;
    std::thread thread_;
    udp::endpoint remote_endpoint_;
    std::array<char, 65536> recv_buffer_;
    std::set<udp::endpoint> clients_;
    std::mutex clients_mutex_;
    std::atomic<bool> running_;
};

#endif // UDP_SERVER_H
