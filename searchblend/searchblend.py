import sys
import socket
import argparse
import threading
from cshogi.cli import Engine

LISTEN_HOST = "127.0.0.1"
LISTEN_PORT = 8765
engine_path = ""
engine_options = []
RECV_PACKET_SIZE = 128

def process_connection(conn: socket.socket, address):
    print("connected by", address)
    pending = b""
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
        recv_data = conn.recv(RECV_PACKET_SIZE * 256)#128bytes/pos * max 256batch
        if len(recv_data) == 0:
            break
        pending += recv_data
        result_concat = b""
        while len(pending) >= RECV_PACKET_SIZE:
            sfen = pending[:RECV_PACKET_SIZE].rstrip(b"\0").decode("ascii")# "sfen lnsgkgsnl/1r5b1/pppppp1pp/6p2/9/4P4/PPPP1PPPP/1B5R1/LNSGKGSNL b - 3"
            pending = pending[RECV_PACKET_SIZE:]
            # print(sfen)
            # パフォーマンス測定用ダミー返信
            # result_concat += b"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
            # continue
            engine.position(sfen=sfen)
            latest_cp = 0
            bestmove, _ponder = engine.go(byoyomi=100, listener=listen_info)
            # print("bestmove", bestmove, latest_cp)
            result_concat += (bestmove.ljust(8, "\0") + str(latest_cp).ljust(8, "\0")).encode("ascii")
        conn.sendall(result_concat)
    print("closed", address)
    conn.close()
    engine.quit()

def main():
    global engine_path, engine_options
    parser = argparse.ArgumentParser()
    parser.add_argument("engine")
    parser.add_argument("--options", default="")
    args = parser.parse_args()
    engine_path = args.engine
    for entry in args.options.split(","):
        engine_options.append(entry.split(":"))
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind((LISTEN_HOST, LISTEN_PORT))
        sock.listen()
        print("listening")
        while True:
            conn, address = sock.accept()
            th = threading.Thread(target=process_connection, args=(conn, address))
            th.start()

if __name__ == "__main__":
    sys.exit(int(main() or 0))
