
#include "external_eval.h"

ExternalEval::ExternalEval(const char* host, int port) :conn(INVALID_SOCKET)
{
	/* sockaddr_in 構造体 */
	struct sockaddr_in dstAddr;
	memset(&dstAddr, 0, sizeof(dstAddr));
	dstAddr.sin_port = htons(port);
	dstAddr.sin_family = AF_INET;
	dstAddr.sin_addr.s_addr = inet_addr(host);

	/* ソケット生成 */
	conn = socket(AF_INET, SOCK_STREAM, 0);

	/* 接続 */
	connect(conn, (struct sockaddr*)&dstAddr, sizeof(dstAddr));
}

ExternalEval::~ExternalEval()
{
	if (conn != INVALID_SOCKET) {
		closesocket(conn);
		conn = INVALID_SOCKET;
	}
}

void ExternalEval::send_sfens(std::vector<std::string>& sfens, size_t len)
{
	if (conn == INVALID_SOCKET) {
		return;
	}
	const size_t CONSTANT_PACKET_SIZE = 128;
	size_t data_len = len * CONSTANT_PACKET_SIZE;
	const auto data = new char[data_len];
	for (size_t i = 0; i < len; i++)
	{
		strncpy(&data[i * CONSTANT_PACKET_SIZE], sfens[i].c_str(), CONSTANT_PACKET_SIZE);
	}
	size_t sent_size = 0;
	while (sent_size < data_len) {
		int cur_sent = send(conn, &data[sent_size], data_len - sent_size, 0);
		if (cur_sent <= 0) {
			conn = INVALID_SOCKET;
			return;
		}
		sent_size += (size_t)cur_sent;
	}
	delete[] data;
}

std::shared_ptr<ExternalEvalResult[]> ExternalEval::receive_result(size_t len)
{
	if (conn == INVALID_SOCKET) {
		return nullptr;
	}
	const size_t data_len = sizeof(ExternalEvalResult) * len;

	const auto buf = std::shared_ptr<ExternalEvalResult[]>(new ExternalEvalResult[len]);
	const auto data = buf.get();
	size_t recv_size = 0;
	while (recv_size < data_len) {
		int cur_recv = recv(conn, (char*)&data[recv_size], data_len - recv_size, 0);
		if (cur_recv <= 0) {
			conn = INVALID_SOCKET;
			return nullptr;
		}
		recv_size += (size_t)cur_recv;
	}
	return buf;
}

bool ExternalEval::ok() const
{
	return conn != INVALID_SOCKET;
}

#ifdef _WIN64
bool ExternalEval::wsa_startup()
{
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0)
	{
		return false;
	}
	return false;
}
#else
bool ExternalEval::wsa_startup()
{
	return true;
}
#endif
