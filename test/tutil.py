import atexit
import os
import signal
import subprocess as sp
import time

import grpc

import ateles_pb2
import ateles_pb2_grpc


SERVER = None


def ensure_server_running():
    global SERVER
    if SERVER is not None:
        return
    if "EXTERNAL_ATELES" in os.environ:
        return
    path = os.path.join(os.path.dirname(__file__), "..", "build", "ateles")
    path = os.path.abspath(path)
    SERVER = sp.Popen(path)
    time.sleep(1)


@atexit.register
def close_server():
    global SERVER
    if SERVER is None:
        return
    SERVER.send_signal(signal.SIGINT)


def connect(reset=False):
    ensure_server_running()
    channel = grpc.insecure_channel('localhost:50051')
    stub = ateles_pb2_grpc.AtelesStub(channel)
    if reset:
        req = ateles_pb2.ResetRequest();
        stub.Reset(req)
    return stub
