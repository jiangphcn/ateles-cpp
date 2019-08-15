# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

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
