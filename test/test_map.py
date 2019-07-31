

import json
import os
import subprocess as sp
import time

import grpc
import pytest

import ateles_pb2
import ateles_pb2_grpc


@pytest.fixture(scope="module")
def stub():
    print "Creating connection"
    path = os.path.join(os.path.dirname(__file__), "..", "build", "ateles")
    path = os.path.abspath(path)
    #pipe = sp.Popen(path)
    try:
        time.sleep(1)
        with grpc.insecure_channel('localhost:50051') as channel:
            stub = ateles_pb2_grpc.AtelesStub(channel)
            yield stub
    finally:
        #pipe.kill()
        pass


def test_create_context_with_map_funs(stub):
    req = ateles_pb2.CreateContextRequest(context_id="foo");
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="foo", map_funs=[
            "function(doc) {emit(doc.value, null);}"
        ])
    ret = stub.AddMapFuns(req)
    print dir(ret)


def test_map_doc(stub):
    def gen_docs():
        for i in range(0, 4):
            yield ateles_pb2.MapDocsRequest(
                    context_id="foo",
                    map_id="%d" % i,
                    doc='{"value": %s}' % i
                )
    for idx, resp in enumerate(stub.MapDocs(gen_docs())):
        assert resp.ok
        assert resp.map_id == "%d" % idx
        assert json.loads(resp.result) == [[[idx, None]]]
