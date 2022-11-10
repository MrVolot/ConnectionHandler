#pragma once
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <functional>

template <typename ConnectionClass>
class IConnectionHandler {
public:
	virtual void setAsyncReadCallback(std::function <void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler>, const boost::system::error_code&, size_t)> callback) = 0;
	virtual void setWriteCallback(std::function <void(ConnectionClass* obj, std::shared_ptr<IConnectionHandler>, const boost::system::error_code&, size_t)> callback) = 0;
	virtual void callAsyncRead() = 0;
	virtual void callRead() = 0;
	virtual void callWrite(const std::string& str) = 0;
	virtual boost::asio::ip::tcp::socket& getSocket() = 0;
	virtual std::unique_ptr<boost::asio::streambuf>& getStrBuf() = 0;
	virtual void resetStrBuf() = 0;
	virtual ConnectionClass& getConnector() = 0;
	virtual std::string getData() = 0;
};