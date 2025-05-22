//udp_server.cpp
#include "udp_server.h"

UDPServer::UDPServer(unsigned short port)
    : io_context_(),
    work_(boost::asio::make_work_guard(io_context_)),
    port_(port),
    socket_(io_context_, udp::endpoint(udp::v4(), port)),
    running_(false)
{
    qDebug() << "[UDPServer] Инициализация сервера на порту" << port_;
}

UDPServer::~UDPServer() {
    stop();
    qDebug() << "[UDPServer] Деструктор вызван";
}
void UDPServer::start() {
    if (running_) {
        qDebug() << "[UDPServer] Сервер уже запущен";
        return;
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.clear();
        qDebug() << "[UDPServer] Список клиентов очищен при запуске сервера";
    }

    io_context_.restart();

    running_ = true;
    qDebug() << "[UDPServer] Запуск сервера";

    // Запускаем поток
    thread_ = std::thread(&UDPServer::run, this);
}

// В stop() сбрасываем work_ и останавливаем io_context
void UDPServer::stop() {
    if (!running_) {
        qDebug() << "[UDPServer] Сервер уже остановлен";
        return;
    }
    qDebug() << "[UDPServer] Остановка сервера";
    running_ = false;
    io_context_.stop();

    // Очистка списка клиентов при остановке
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.clear();
        qDebug() << "[UDPServer] Список клиентов очищен при остановке сервера";
    }

    // Освобождаем work_guard, чтобы завершить run()
    work_.reset();

    if (thread_.joinable()) {
        thread_.join();
        qDebug() << "[UDPServer] Поток сервера завершен";
    }
}

void UDPServer::run() {
    qDebug() << "[UDPServer] Внутренний цикл запускается";

    // Запуск приема данных
    start_receive();

    try {
        io_context_.run(); // Блокируется, пока не вызовется stop()
        qDebug() << "[UDPServer] io_context_ завершил работу";
    } catch (const std::exception& e) {
        qDebug() << "[UDPServer] Исключение в run(): " << e.what();
    }
}

void UDPServer::start_receive() {
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_),
        remote_endpoint_,
        [this](const boost::system::error_code& ec, std::size_t bytes_recvd) {
            handle_receive(ec, bytes_recvd);
        });
    qDebug() << "[UDPServer] Запущен асинхронный прием данных";
}

void UDPServer::handle_receive(const boost::system::error_code& error, std::size_t length) {
    if (!running_) {
        qDebug() << "[UDPServer] Получена receive, но сервер остановлен";
        return;
    }
    if (!error && length > 0) {
        auto sender_endpoint = remote_endpoint_;
        QString client_ip = QString::fromStdString(sender_endpoint.address().to_string());
        quint16 client_port = sender_endpoint.port();

        std::string msg(recv_buffer_.data(), length);

        if (msg == "DISCONNECT") {
            // Удаление клиента из списка
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                auto it = clients_.find(sender_endpoint);
                if (it != clients_.end()) {
                    clients_.erase(it);
                    qDebug() << "[UDPServer] Клиент отключен: " << client_ip << ":" << client_port << " (DISCONNECT)";
                } else {
                    qDebug() << "[UDPServer] Отключение клиента, которого нет в списке: " << client_ip << ":" << client_port;
                }
                qDebug() << "[UDPServer] Текущий список клиентов после отключения:";
                for (const auto& client_ep : clients_) {
                    QString ip = QString::fromStdString(client_ep.address().to_string());
                    quint16 port = client_ep.port();
                    qDebug() << "  - " << ip << ":" << port;
                }
            }
        } else if (msg == "CONNECT") {
            // Регистрация нового клиента
            registerClient(sender_endpoint);
        } else {
            qDebug() << "[UDPServer] Получено сообщение от" << client_ip << ":" << client_port << ", размер" << length;
            sendToClients(recv_buffer_.data(), length, sender_endpoint);
        }
    } else {
        qDebug() << "[UDPServer] Ошибка приема:" << error.message();
    }
    // Продолжаем слушать
    start_receive();
}
void UDPServer::sendToClients(const char* data, std::size_t length, const udp::endpoint& exclude_endpoint) {
    std::set<udp::endpoint> clients_copy;
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_copy = clients_; // копируем список клиентов
    }
    for (const auto& client : clients_copy) {
        if (client != exclude_endpoint) {
            qDebug() << "[UDPServer] Отправка сообщения клиенту"
                     << QString::fromStdString(client.address().to_string())
                     << ":" << client.port();
            socket_.async_send_to(
                boost::asio::buffer(data, length),
                client,
                [client](const boost::system::error_code& error, std::size_t bytes_sent) {
                    if (error) {
                        qDebug() << "[UDPServer] Ошибка при отправке:" << error.message();
                    } else {
                        qDebug() << "[UDPServer] Отправлено" << bytes_sent << "байт клиенту"
                                 << QString::fromStdString(client.address().to_string())
                                 << ":" << client.port();
                    }
                });
        }
    }
}

bool UDPServer::isRunning() const {
    return running_;
}
void UDPServer::registerClient(const udp::endpoint& client_endpoint) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto inserted = clients_.insert(client_endpoint).second;
    if (inserted) {
        QString ip = QString::fromStdString(client_endpoint.address().to_string());
        quint16 port = client_endpoint.port();
        qDebug() << "[UDPServer] Новый клиент зарегистрирован:" << ip << ":" << port;
    } else {
        QString ip = QString::fromStdString(client_endpoint.address().to_string());
        quint16 port = client_endpoint.port();
        qDebug() << "[UDPServer] Клиент уже зарегистрирован:" << ip << ":" << port;
    }
}

void UDPServer::unregisterClient(const udp::endpoint& client_endpoint) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients_.find(client_endpoint);
    if (it != clients_.end()) {
        QString ip = QString::fromStdString(it->address().to_string());
        quint16 port = it->port();
        clients_.erase(it);
        qDebug() << "[UDPServer] Клиент отключен:" << ip << ":" << port;
    } else {
        QString ip = QString::fromStdString(client_endpoint.address().to_string());
        quint16 port = client_endpoint.port();
        qDebug() << "[UDPServer] Попытка отключить несуществующего клиента:" << ip << ":" << port;
    }
}
