#pragma once

#include "IConnectionHandler.h"

template<typename ConnectionClass>
class ConnectionHandler : public std::enable_shared_from_this<ConnectionHandler<ConnectionClass>>, public IConnectionHandler<ConnectionClass>{
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service& service_;
    const size_t msgLength_{ 1024 };
	std::unique_ptr<boost::asio::streambuf> strBuf_; 
    std::function <void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> readCallback_;
    std::function <void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> asyncReadCallback_;
    std::function <void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> writeCallback_;
	ConnectionClass& caller_;
	static constexpr auto delimiter{ "\r\n\r\n" };
public:
    ConnectionHandler(boost::asio::io_service& service, ConnectionClass& caller);
    ConnectionHandler(const ConnectionHandler& rhs) = delete;
	virtual ~ConnectionHandler();
    void setAsyncReadCallback(std::function <void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> callback) override;
    void setWriteCallback(std::function <void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> callback) override;
    void callAsyncRead() override;
    void callWrite(const std::string& str) override;
    boost::asio::ip::tcp::socket& getSocket() override;
    std::unique_ptr<boost::asio::streambuf>& getStrBuf() override;
    void resetStrBuf();
	ConnectionClass& getConnector() override;
	void callRead() override;
	std::string getData();
	void callAsyncHandshake() override {};
};

template<typename T>
ConnectionHandler<T>::ConnectionHandler(boost::asio::io_service& service, T& caller) : socket_{ service },
service_{ service }, strBuf_{ new boost::asio::streambuf }, caller_{caller}
{

}
template<typename ConnectionClass>
ConnectionHandler<ConnectionClass>::~ConnectionHandler()
{
	socket_.close();
}

template<typename T>
void ConnectionHandler<T>::setAsyncReadCallback(std::function<void(T* obj, std::shared_ptr<IConnectionHandler<T>>, const boost::system::error_code&, size_t)> callback)
{
	asyncReadCallback_ = callback;
}
template<typename T>
void ConnectionHandler<T>::setWriteCallback(std::function<void(T* obj, std::shared_ptr<IConnectionHandler<T>>, const boost::system::error_code&, size_t)> callback)
{
	writeCallback_ = callback;
}
template<typename T>
void ConnectionHandler<T>::callAsyncRead()
{
	boost::asio::async_read_until(socket_, *strBuf_.get(), delimiter, boost::bind(ConnectionHandler<T>::asyncReadCallback_,
		&caller_,
		this->shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}
template<typename T>
void ConnectionHandler<T>::callWrite(const std::string& str)
{
	std::string strToSend{ str };
	strToSend.append(delimiter);
	socket_.async_write_some(boost::asio::buffer(strToSend.c_str(), strToSend.size()), boost::bind(ConnectionHandler<T>::writeCallback_,
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
void ConnectionHandler<T>::resetStrBuf()
{
	strBuf_.reset(new boost::asio::streambuf);
}

template<typename ConnectionClass>
ConnectionClass& ConnectionHandler<ConnectionClass>::getConnector()
{
	return caller_;
}

template<typename ConnectionClass>
void ConnectionHandler<ConnectionClass>::callRead()
{
	//socket_.read_some(boost::asio::buffer(mutableBuffer_));
}

template<typename ConnectionClass>
std::string ConnectionHandler<ConnectionClass>::getData() {
	std::string data{ boost::asio::buffer_cast<const char*> (strBuf_->data()) };
	return data.substr(0, data.length() - strlen(delimiter));
}