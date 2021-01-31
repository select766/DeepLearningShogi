import sys
import socket
import argparse
import threading
import queue
import time
from cshogi.cli import Engine

LISTEN_HOST = "127.0.0.1"
LISTEN_PORT = 8765
engine_path = ""
engine_options = []
RECV_PACKET_SIZE = 128
request_queue = queue.SimpleQueue()# connection_idx, result_idx, sfen
response_queues = {} # connection_idx: (result_idx, packet)

def go_pool():
    engine = Engine(engine_path)
    engine.usi()
    for k, v in engine_options:
        engine.setoption(k, v)
    engine.isready()
    engine.usinewgame()
    latest_cp = 0
    def listen_info(message: str):
        nonlocal latest_cp
        if message.startswith("info"):
            elements = message.split(" ")
            try:
                idx = elements.index("score")#score cp 123
                if elements[idx + 1] == "cp":
                    latest_cp = int(elements[idx + 2])
                else:
                    #mate
                    if elements[idx + 2].startswith("-"):
                        latest_cp = -99999#詰みまでの手数不明の場合、"-2"ではなく"-"という値が返る場合がある
                    else:
                        latest_cp = 99999
            except ValueError:
                pass
    while True:
        connection_idx, result_idx, sfen = request_queue.get()
        engine.position(sfen=sfen)
        latest_cp = 0
        bestmove, _ponder = engine.go(byoyomi=100, listener=listen_info)
        result_packet = (bestmove.ljust(8, "\0") + str(latest_cp).ljust(8, "\0")).encode("ascii")
        response_queues[connection_idx].put((result_idx, result_packet))


def process_connection(conn: socket.socket, address, connection_idx: int):
    print("connected by", address)
    pending = b""
    response_queue = queue.SimpleQueue()
    response_queues[connection_idx] = response_queue
    go_ctr = 0
    go_time_sum = 0.0
    while True:
        recv_data = conn.recv(RECV_PACKET_SIZE * 256)#128bytes/pos * max 256batch
        if len(recv_data) == 0:
            break
        pending += recv_data
        request_count = 0
        go_start = time.perf_counter()
        while len(pending) >= RECV_PACKET_SIZE:
            sfen = pending[:RECV_PACKET_SIZE].rstrip(b"\0").decode("ascii")# "sfen lnsgkgsnl/1r5b1/pppppp1pp/6p2/9/4P4/PPPP1PPPP/1B5R1/LNSGKGSNL b - 3"
            pending = pending[RECV_PACKET_SIZE:]
            request_queue.put((connection_idx, request_count, sfen))
            request_count += 1
        if request_count > 0:
            ordered_results = [None] * request_count
            recv_count = 0
            while recv_count < request_count:
                result_idx, result_packet = response_queue.get()
                ordered_results[result_idx] = result_packet
                recv_count += 1
            go_end = time.perf_counter()
            go_ctr += request_count
            go_time_sum += go_end - go_start
            result_concat = b"".join(ordered_results)
            conn.sendall(result_concat)
    print("closed", address)
    conn.close()
    del response_queues[connection_idx]#スレッドセーフではないが、並列対局しない限り問題にならない
    print("go time sum", go_time_sum, "positions", go_ctr, "avg", go_time_sum / go_ctr)# あくまでこのコネクションから見た処理時間。複数コネクションが干渉する。

def main():
    global engine_path, engine_options
    print("Press Ctrl+Break to quit (Windows)")
    parser = argparse.ArgumentParser()
    parser.add_argument("engine")
    parser.add_argument("--options", default="")
    parser.add_argument("--pool", type=int, default=1)
    parser.add_argument("--host", default=LISTEN_HOST)
    parser.add_argument("--port", default=LISTEN_PORT, type=int)
    args = parser.parse_args()
    engine_path = args.engine
    for entry in args.options.split(","):
        engine_options.append(entry.split(":"))
    for i in range(args.pool):
        th = threading.Thread(target=go_pool)
        th.start()
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind((args.host, args.port))
        sock.listen()
        print("listening")
        next_connection_idx = 0
        while True:
            conn, address = sock.accept()
            th = threading.Thread(target=process_connection, args=(conn, address, next_connection_idx))
            th.start()
            next_connection_idx += 1

if __name__ == "__main__":
    sys.exit(int(main() or 0))

