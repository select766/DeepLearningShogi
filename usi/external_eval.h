#pragma once

#include <vector>
#include <string>
#include <memory>

#ifdef _WIN64
#define NOMINMAX
#include <Windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define BOOL int
#endif

// #define EXTERNAL_PERF_COUNT
void external_perf_start();
void external_perf_end();
void external_perf_print();

struct ExternalEvalResult {
	// Å‘Pè‚ÌUSI•\‹L
	char move_usi[8];
	// •]‰¿’l‚Ì•¶š—ñ
	char value_num[8];
};

class ExternalEval {
public:
	ExternalEval(const char* host = "127.0.0.1", int port = 8765);
	~ExternalEval();
	void send_sfens(std::vector<std::string>& sfens, size_t len);
	void send_sfen(std::string& sfen);
	std::shared_ptr<ExternalEvalResult[]> receive_result(size_t len);
	bool ok() const;
	static bool wsa_startup();
private:
	SOCKET conn;
};
