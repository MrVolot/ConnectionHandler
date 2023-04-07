#pragma once

#include "IConnectionHandler.h"
#include <boost/asio/ssl.hpp>
#include <iostream>

enum ConnectionHandlerType {
    LOGIN_SERVER,
    SERVER,
    CLIENT
};

template <typename ConnectionClass, ConnectionHandlerType type>
class HttpsConnectionHandler : public std::enable_shared_from_this<HttpsConnectionHandler<ConnectionClass, type>>,
    public IConnectionHandler<ConnectionClass> {
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    boost::asio::io_service& service_;
    const size_t msgLength_{ 1024 };
    std::unique_ptr<boost::asio::streambuf> strBuf_;
    std::function<void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> readCallback_;
    std::function<void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> asyncReadCallback_;
    std::function<void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> writeCallback_;
    ConnectionClass& caller_;
    static constexpr auto delimiter{ "\r\n\r\n" };

public:
    HttpsConnectionHandler(boost::asio::io_service& service, ConnectionClass& caller, boost::asio::ssl::context& ssl_context);
    virtual ~HttpsConnectionHandler();
    void setAsyncReadCallback(std::function<void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> callback) override;
    void setWriteCallback(std::function<void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> callback) override;
    void callAsyncRead() override;
    void callRead() override;
    void callWrite(const std::string& str) override;
    boost::asio::ip::tcp::socket& getSocket() override;
    std::unique_ptr<boost::asio::streambuf>& getStrBuf() override;
    void resetStrBuf() override;
    ConnectionClass& getConnector() override;
    std::string getData() override;
    void callAsyncHandshake() override;
};

template <typename ConnectionClass, ConnectionHandlerType type>
HttpsConnectionHandler<ConnectionClass, type>::HttpsConnectionHandler(boost::asio::io_service& service, ConnectionClass& caller, boost::asio::ssl::context& ssl_context)
    : socket_{ service, ssl_context }, service_{ service }, strBuf_{ new boost::asio::streambuf }, caller_{ caller } {
}

template <typename ConnectionClass, ConnectionHandlerType type>
HttpsConnectionHandler<ConnectionClass, type>::~HttpsConnectionHandler() {
    socket_.lowest_layer().close();
}

template <typename ConnectionClass, ConnectionHandlerType type>
void HttpsConnectionHandler<ConnectionClass, type>::setAsyncReadCallback(std::function<void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> callback) {
    asyncReadCallback_ = callback;
}

template <typename ConnectionClass, ConnectionHandlerType type>
void HttpsConnectionHandler<ConnectionClass, type>::setWriteCallback(std::function<void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> callback) {
    writeCallback_ = callback;
}

template <typename ConnectionClass, ConnectionHandlerType type>
void HttpsConnectionHandler<ConnectionClass, type>::callAsyncRead() {
    boost::asio::async_read_until(socket_, *strBuf_.get(), delimiter, boost::bind(asyncReadCallback_, &caller_, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

template <typename ConnectionClass, ConnectionHandlerType type>
void HttpsConnectionHandler<ConnectionClass, type>::callRead() {
    // Implement synchronous read if needed
}

template <typename ConnectionClass, ConnectionHandlerType type>
void HttpsConnectionHandler<ConnectionClass, type>::callWrite(const std::string& str) {
    std::string strToSend{ str };
    strToSend.append(delimiter);
    boost::asio::async_write(socket_, boost::asio::buffer(strToSend.c_str(), strToSend.size()), boost::bind(writeCallback_, &caller_, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

template <typename ConnectionClass, ConnectionHandlerType type>
boost::asio::ip::tcp::socket& HttpsConnectionHandler<ConnectionClass, type>::getSocket() {
    return socket_.next_layer();
}

template <typename ConnectionClass, ConnectionHandlerType type>
std::unique_ptr<boost::asio::streambuf>& HttpsConnectionHandler<ConnectionClass, type>::getStrBuf() {
    return strBuf_;
}

template <typename ConnectionClass, ConnectionHandlerType type>
void HttpsConnectionHandler<ConnectionClass, type>::resetStrBuf() {
    strBuf_.reset(new boost::asio::streambuf);
}

template <typename ConnectionClass, ConnectionHandlerType type>
ConnectionClass& HttpsConnectionHandler<ConnectionClass, type>::getConnector() {
    return caller_;
}

template <typename ConnectionClass, ConnectionHandlerType type>
std::string HttpsConnectionHandler<ConnectionClass, type>::getData() {
    std::string data{ boost::asio::buffer_cast<const char*>(strBuf_->data()) };
    return data.substr(0, data.length() - strlen(delimiter));
}

template<typename ConnectionClass, ConnectionHandlerType type>
void HttpsConnectionHandler<ConnectionClass, type>::callAsyncHandshake() {

    if constexpr (type == ConnectionHandlerType::LOGIN_SERVER || type == ConnectionHandlerType::SERVER) {
        socket_.async_handshake(boost::asio::ssl::stream_base::server,
            [self = this->shared_from_this()](const boost::system::error_code& error) {
                if (!error) {
                    std::cout << "Handshake successful." << std::endl;
                    self->callAsyncRead();
                }
                else {
                    std::cout << "Handshake error: " << error.message() << std::endl;
                }
            });
    }
    else if constexpr (type == ConnectionHandlerType::CLIENT) {
        socket_.async_handshake(boost::asio::ssl::stream_base::client,
            [self = this->shared_from_this()](const boost::system::error_code& error) {
                if (!error) {
                    std::cout << "Handshake successful." << std::endl;
                    self->callAsyncRead();
                    self->getConnector().processAfterHandshake();
                }
                else {
                    std::cout << "Handshake error: " << error.message() << std::endl;
                }
            });
    }
}