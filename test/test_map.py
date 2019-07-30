

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


def test_create_context(stub):
    ctx_info = ateles_pb2.MapContextInfo(name="foo", lib="", map_funs=[
            "function(doc) {emit(doc.value, null);}"
        ])
    ret = stub.CreateMapContext(ctx_info)
    print ret


def test_map_doc(stub):
    def gen_docs():
        for i in range(1, 5):
            body = '{"value": %s}' % i
            doc = ateles_pb2.Doc(ref="foo", body=body)
            yield doc
    resps = stub.MapDocs(gen_docs())
    for resp in resps:
        print resp.results
    assert 1 == 0