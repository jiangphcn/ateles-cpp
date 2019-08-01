import atexit
import os
import signal
import subprocess as sp
import time


SERVER = None


def ensure_server_running():
    global SERVER
    if SERVER is not None:
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
