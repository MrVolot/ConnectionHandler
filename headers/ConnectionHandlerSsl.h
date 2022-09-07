#pragma once

#include "IConnectionHandler.h"
#include <boost/asio/ssl.hpp>

template<typename ConnectionClass>
class ConnectionHandlerSsl : public std::enable_shared_from_this<ConnectionHandlerSsl<ConnectionClass>>, public IConnectionHandler<ConnectionClass> {
	boost::asio::ssl::context context_;
	boost::asio::ip::tcp::socket rawSocket_;
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
	boost::asio::io_service& service_;
	const size_t msgLength_{ 1024 };
	std::unique_ptr<boost::asio::streambuf> strBuf_;
	boost::asio::streambuf::mutable_buffers_type mutableBuffer_;
	std::function <void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> asyncReadCallback_;
	std::function <void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> writeCallback_;
	ConnectionClass& caller_;

	std::string getPassword();
public:
	ConnectionHandlerSsl(boost::asio::io_service& service, ConnectionClass& caller);
	ConnectionHandlerSsl(const ConnectionHandlerSsl& rhs) = delete;
	virtual ~ConnectionHandlerSsl();
	void setAsyncReadCallback(std::function <void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> callback) override;
	void setWriteCallback(std::function <void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler<ConnectionClass>>, const boost::system::error_code&, size_t)> callback) override;
	void callAsyncRead() override;
	void callWrite(const std::string& str) override;
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& getSocket() override;
	std::unique_ptr<boost::asio::streambuf>& getStrBuf() override;
	void setMutableBuffer() override;
	ConnectionClass& getConnector() override;
	void callRead() override;
	bool serverHandShake();
};

template<typename T>
ConnectionHandlerSsl<T>::ConnectionHandlerSsl(boost::asio::io_service& service, T& caller) :
	rawSocket_{ service },
	context_{ boost::asio::ssl::context::sslv23 },
	socket_{ std::move(rawSocket_), context_ },
	service_{ service },
	strBuf_{ new boost::asio::streambuf },
	mutableBuffer_{ strBuf_->prepare(msgLength_) },
	caller_{ caller }
{
	context_.set_options(
		boost::asio::ssl::context::default_workarounds
		| boost::asio::ssl::context::no_sslv2
		| boost::asio::ssl::context::single_dh_use);
	context_.set_password_callback(std::bind(&ConnectionHandlerSsl<T>::getPassword, this));
	context_.use_certificate_chain_file("cert.pem");
	context_.use_private_key_file("key.pem", boost::asio::ssl::context::pem);
	context_.use_tmp_dh_file("dhparams.pem");
}
template<typename ConnectionClass>
std::string ConnectionHandlerSsl<ConnectionClass>::getPassword()
{
	return { "pass" };
}
template<typename ConnectionClass>
ConnectionHandlerSsl<ConnectionClass>::~ConnectionHandlerSsl()
{

}

template<typename T>
void ConnectionHandlerSsl<T>::setAsyncReadCallback(std::function<void(T* obj, std::shared_ptr<IConnectionHandler<T>>, const boost::system::error_code&, size_t)> callback)
{
	asyncReadCallback_ = callback;
}
template<typename T>
void ConnectionHandlerSsl<T>::setWriteCallback(std::function<void(T* obj, std::shared_ptr<IConnectionHandler<T>>, const boost::system::error_code&, size_t)> callback)
{
	writeCallback_ = callback;
}
template<typename T>
void ConnectionHandlerSsl<T>::callAsyncRead()
{
	socket_.async_read_some(boost::asio::buffer(mutableBuffer_), boost::bind(ConnectionHandlerSsl<T>::asyncReadCallback_,
		&caller_,
		this->shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}
template<typename T>
void ConnectionHandlerSsl<T>::callWrite(const std::string& str)
{
	socket_.async_write_some(boost::asio::buffer(str.c_str(), str.size()), boost::bind(ConnectionHandlerSsl<T>::writeCallback_,
		&caller_,
		this->shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}
template<typename T>
boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& ConnectionHandlerSsl<T>::getSocket()
{
	return socket_;
}
template<typename T>
std::unique_ptr<boost::asio::streambuf>& ConnectionHandlerSsl<T>::getStrBuf()
{
	return strBuf_;
}
template<typename T>
void ConnectionHandlerSsl<T>::setMutableBuffer()
{
	mutableBuffer_ = strBuf_->prepare(msgLength_);
}

template<typename ConnectionClass>
ConnectionClass& ConnectionHandlerSsl<ConnectionClass>::getConnector()
{
	return caller_;
}

template<typename ConnectionClass>
void ConnectionHandlerSsl<ConnectionClass>::callRead()
{
	socket_.read_some(boost::asio::buffer(mutableBuffer_));
}

template<typename ConnectionClass>
bool ConnectionHandlerSsl<ConnectionClass>::serverHandShake()
{
	socket_.async_handshake(boost::asio::ssl::stream_base::server,
		[](const boost::system::error_code& error)
		{
			if (!error)
			{
				return true;
			}
			return false;
		});
	return true;
}
