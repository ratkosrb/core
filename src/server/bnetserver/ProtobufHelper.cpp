#include <chrono>
#include <errno.h>
#include <tuple>

#include "Log.h"
#include "ProtobufHelper.h"

constexpr size_t PROTOBUF_HEADER_LENGTH = 4;
constexpr size_t MAX_TCP_BLOCK_SIZE = 10 * 1024 * 1024 * 8;

template <typename T>
std::shared_ptr<T> make_shared_array(size_t size)
{
    return std::shared_ptr<T>(new T[size], std::default_delete<T[]>());
}

const std::tuple<std::shared_ptr<char>, size_t> ProtobufHelper::serialize(const google::protobuf::Message &msg)
{
	const static char fname[] = "ProtobufHelper::serialize() ";

	const auto msgLength = msg.ByteSize();
	const auto totalLength = PROTOBUF_HEADER_LENGTH + msgLength;
	const auto buffer = make_shared_array<char>(totalLength);
	*((uint32_t *)buffer.get()) = htonl(msgLength); // host to network byte order
	if (!msg.SerializeToArray(buffer.get() + 4, msgLength))
	{
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "%s %s", fname, msg.DebugString().c_str());
		return std::make_tuple(nullptr, 0);
	}
	return std::make_tuple(buffer, totalLength); // return buffer and length pair
}

bool ProtobufHelper::deserialize(google::protobuf::Message &msg, const char *data, int dataSize)
{
	const static char fname[] = "ProtobufHelper::deserialize() ";

	// De-Serialize
	if (!msg.ParseFromArray(data, dataSize))
	{
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "%s ParseFromCodedStream failed with error : %s", fname, msg.DebugString().c_str());
		return false;
	}
	return true;
}

const std::tuple<std::shared_ptr<char>, int> ProtobufHelper::readMessageBlock(const ACE_SSL_SOCK_Stream &socket)
{
	const static char fname[] = "ProtobufHelper::readMessageBlock() ";
    sLog.Out(LOG_BASIC, LOG_LVL_DEBUG, "%s entered", fname);

	ssize_t recvReturn = 0;
	const auto bodySize = readMsgHeader(socket, recvReturn);
	if (bodySize <= 0)
	{
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "%s parse header length with error : %s", fname, std::strerror(errno));
		return std::make_tuple(nullptr, recvReturn);
	}
	return readBytes(socket, bodySize, recvReturn);
}

int ProtobufHelper::readMsgHeader(const ACE_SSL_SOCK_Stream &socket, ssize_t &recvReturn)
{
	const static char fname[] = "ProtobufHelper::readMsgHeader() ";
	// read header socket data (4 bytes)
	auto result = readBytes(socket, PROTOBUF_HEADER_LENGTH, recvReturn);
	auto data = std::get<0>(result);
	if (recvReturn <= 0)
	{
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "%s read header length failed with error : %s", fname, std::strerror(errno));
		return -1;
	}
	// parse header data (get body length). network to host byte order
	const auto bodySize = ntohl(*((int *)(data.get()))); // host to network byte order
    sLog.Out(LOG_BASIC, LOG_LVL_DEBUG, "%s read length : %u from header", fname, bodySize);
	if (bodySize > MAX_TCP_BLOCK_SIZE)
	{
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "%s read data size reached limitation, aborting connection", fname);
		return -1;
	}
	return bodySize;
}

const std::tuple<std::shared_ptr<char>, int> ProtobufHelper::readBytes(const ACE_SSL_SOCK_Stream &socket, size_t bodySize, ssize_t &recvReturn)
{
	const static char fname[] = "ProtobufHelper::readBytes() ";

	// read socket data with given length
	const auto bufferSize = bodySize;
	auto bodyBuffer = make_shared_array<char>(bufferSize);
	// https://www.demo2s.com/c/c-if-errno-eintr-fiag.html
	// https://programmerall.com/article/5562684780/#:~:text=When%20a%20certain%20signal%20is%20caught%2C%20the%20system,system%20calls%20that%20may%20block%20the%20process%20forever.
	errno = 0;
	size_t totalRecieved = 0;
	recvReturn = socket.recv_n(bodyBuffer.get(), bufferSize, 0, &totalRecieved);
	while (totalRecieved < bufferSize && errno == EINTR)
	{
		size_t transfered = 0;
		recvReturn = socket.recv_n(bodyBuffer.get() + totalRecieved, bufferSize - totalRecieved, 0, &transfered);
		totalRecieved += transfered;
	}
	if (bufferSize == totalRecieved)
		recvReturn = totalRecieved;
	if (socket.get_handle() != ACE_INVALID_HANDLE && recvReturn <= 0)
	{
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "%s read body socket data failed with error: ", fname, std::strerror(errno));
		return std::make_tuple(nullptr, recvReturn);
	}
    sLog.Out(LOG_BASIC, LOG_LVL_DEBUG, "%s read message block data with length: %u", fname, bufferSize);
	return std::make_tuple(bodyBuffer, recvReturn);
}
