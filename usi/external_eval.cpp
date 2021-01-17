
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
	//if (true) {
	//	// パフォーマンス測定用
	//	return;
	//}
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

#include <iostream>

void ExternalEval::send_sfen(std::string& sfen)
{
	//if (true) {
	//	// パフォーマンス測定用
	//	return;
	//}
	if (conn == INVALID_SOCKET) {
		return;
	}
	const size_t CONSTANT_PACKET_SIZE = 128;
	size_t data_len = CONSTANT_PACKET_SIZE;
	const auto data = new char[data_len];
	for (size_t i = 0; i < 1; i++)
	{
		strncpy(&data[i * CONSTANT_PACKET_SIZE], sfen.c_str(), CONSTANT_PACKET_SIZE);
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
	char* data = (char*)buf.get();
	//if (true) {
	//	// パフォーマンス測定用
	//	memset(data, 0, data_len);
	//	return buf;
	//}
	size_t recv_size = 0;
	while (recv_size < data_len) {
		int cur_recv = recv(conn, &data[recv_size], data_len - recv_size, 0);
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


#ifdef EXTERNAL_PERF_COUNT
#include <atomic>
#include <iostream>
static thread_local LARGE_INTEGER thread_perf_start;
static std::atomic_int perf_call_count = 0;
static std::atomic_llong perf_sum = 0;
void external_perf_start() {
	QueryPerformanceCounter(&thread_perf_start);
}
void external_perf_end() {
	LARGE_INTEGER thread_perf_end;
	QueryPerformanceCounter(&thread_perf_end);
	perf_call_count.fetch_add(1);
	perf_sum.fetch_add(thread_perf_end.QuadPart - thread_perf_start.QuadPart);
}
void external_perf_print() {
	LARGE_INTEGER qpf;
	QueryPerformanceFrequency(&qpf);
	double perf_sum_sec = (double)perf_sum / (double)qpf.QuadPart;
	std::cout << "info string performance total " << perf_sum_sec << " sec " << perf_call_count << " calls " << perf_sum_sec / perf_call_count << " sec/call" << std::endl;
}

#else
void external_perf_start() {}
void external_perf_end() {}
void external_perf_print() {}
#endif
