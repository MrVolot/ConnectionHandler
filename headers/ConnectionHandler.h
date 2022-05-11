#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>

template<typename T>
class ConnectionHandler : public std::enable_shared_from_this<ConnectionHandler<T>> {
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service& service_;
    const size_t msgLength_{ 1024 };
    std::unique_ptr<boost::asio::streambuf> strBuf_;
    boost::asio::streambuf::mutable_buffers_type mutableBuffer_;
    std::function <void(T* obj, std::shared_ptr<ConnectionHandler>, const boost::system::error_code&, size_t)> readCallback_;
    std::function <void(T* obj, std::shared_ptr<ConnectionHandler>, const boost::system::error_code&, size_t)> writeCallback_;
	T& caller_;
public:
    ConnectionHandler(boost::asio::io_service& service, T& caller);
    ConnectionHandler(const ConnectionHandler& rhs) = delete;
    void setReadCallback(std::function <void(T* obj, std::shared_ptr<ConnectionHandler>, const boost::system::error_code&, size_t)> callback);
    void setWriteCallback(std::function <void(T* obj, std::shared_ptr<ConnectionHandler>, const boost::system::error_code&, size_t)> callback);
    void callRead();
    void callWrite(const std::string& str);
    boost::asio::ip::tcp::socket& getSocket();
    std::unique_ptr<boost::asio::streambuf>& getStrBuf();
    void setMutableBuffer();
};

template<typename T>
ConnectionHandler<T>::ConnectionHandler(boost::asio::io_service& service, T& caller) : socket_{ service },
service_{ service }, strBuf_{ new boost::asio::streambuf }, mutableBuffer_{ strBuf_->prepare(msgLength_) }, caller_{caller}
{

}
template<typename T>
void ConnectionHandler<T>::setReadCallback(std::function<void(T* obj, std::shared_ptr<ConnectionHandler>, const boost::system::error_code&, size_t)> callback)
{
	readCallback_ = callback;
}
template<typename T>
void ConnectionHandler<T>::setWriteCallback(std::function<void(T* obj, std::shared_ptr<ConnectionHandler>, const boost::system::error_code&, size_t)> callback)
{
	writeCallback_ = callback;
}
template<typename T>
void ConnectionHandler<T>::callRead()
{
	socket_.async_read_some(boost::asio::buffer(mutableBuffer_), boost::bind(ConnectionHandler<T>::readCallback_,
		&caller_,
		this->shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}
template<typename T>
void ConnectionHandler<T>::callWrite(const std::string& str)
{
	socket_.async_write_some(boost::asio::buffer(str.c_str(), str.size()), boost::bind(ConnectionHandler<T>::writeCallback_,
		&caller_,
		this->shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}
template<typename T>
boost::asio::ip::tcp::socket& ConnectionHandler<T>::getSocket()
{
	return socket_;
}
template<typename T>
std::unique_ptr<boost::asio::streambuf>& ConnectionHandler<T>::getStrBuf()
{
	return strBuf_;
}
template<typename T>
void ConnectionHandler<T>::setMutableBuffer()
{
	mutableBuffer_ = strBuf_->prepare(msgLength_);
}