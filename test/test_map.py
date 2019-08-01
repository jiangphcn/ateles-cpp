
import json
import textwrap

import grpc
import pytest

import ateles_pb2
import ateles_pb2_grpc

import tutil


@pytest.fixture(scope="module")
def stub():
    yield tutil.connect(reset=True)


def test_basic_map_doc(stub):
    req = ateles_pb2.CreateContextRequest(context_id="basic_map");
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="basic_map", map_funs=[
            "function(doc) {emit(doc.value, null);}"
        ])
    stub.AddMapFuns(req)

    def gen_docs():
        for i in range(0, 4):
            yield ateles_pb2.MapDocsRequest(
                    context_id="basic_map",
                    map_id="map_id:%d" % i,
                    doc='{"value": %s}' % i
                )
    for idx, resp in enumerate(stub.MapDocs(gen_docs())):
        assert resp.ok
        assert resp.map_id == "map_id:%d" % idx
        assert json.loads(resp.result) == [[[idx, None]]]


def test_no_emit(stub):
    req = ateles_pb2.CreateContextRequest(context_id="no_emit");
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="no_emit", map_funs=[
            "function(doc) {return;}"
        ])
    stub.AddMapFuns(req)

    def gen_docs():
        for i in range(0, 4):
            yield ateles_pb2.MapDocsRequest(
                    context_id="no_emit",
                    map_id="stuff:%d" % i,
                    doc='{"value": %s}' % i
                )
    for idx, resp in enumerate(stub.MapDocs(gen_docs())):
        assert resp.ok
        assert resp.map_id == "stuff:%d" % idx
        assert json.loads(resp.result) == [[]]


def test_multiple_emit(stub):
    req = ateles_pb2.CreateContextRequest(context_id="multiple_emits");
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="multiple_emits", map_funs=[
            textwrap.dedent("""\
            function(doc) {
                emit(doc.value, null);
                emit("bar", doc.value);
            }
            """)
        ])
    stub.AddMapFuns(req)

    def gen_docs():
        for i in range(0, 4):
            yield ateles_pb2.MapDocsRequest(
                    context_id="multiple_emits",
                    map_id="map_id:%d" % i,
                    doc='{"value": %s}' % i
                )
    for idx, resp in enumerate(stub.MapDocs(gen_docs())):
        assert resp.ok
        assert resp.map_id == "map_id:%d" % idx
        row = [[[idx, None], ["bar", idx]]]
        assert json.loads(resp.result) == row


def test_multiple_functions(stub):
    req = ateles_pb2.CreateContextRequest(context_id="multiple_funs");
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="multiple_funs", map_funs=[
            "function(doc) {emit(doc.value, null);}",
            "function(doc) {emit(doc.value * 2, true);}",
            "function(doc) {emit(\"foo\", doc.value);}"
        ])
    stub.AddMapFuns(req)

    def gen_docs():
        for i in range(0, 4):
            yield ateles_pb2.MapDocsRequest(
                    context_id="multiple_funs",
                    map_id="%d" % i,
                    doc='{"value": %s}' % i
                )
    for idx, resp in enumerate(stub.MapDocs(gen_docs())):
        assert resp.ok
        assert resp.map_id == "%d" % idx
        row = [[[idx, None]], [[idx * 2, True]], [["foo", idx]]]
        assert json.loads(resp.result) == row


def test_multiple_functions_mixed_emit(stub):
    req = ateles_pb2.CreateContextRequest(context_id="mixed_emit");
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="mixed_emit", map_funs=[
            textwrap.dedent("""\
            function(doc) {
                emit(doc.value, null);
                emit(false, doc.value);
            }
            """),
            "function(doc) {return;}",
            "function(doc) {emit(true, doc.value)}"
        ])
    stub.AddMapFuns(req)

    def gen_docs():
        for i in range(0, 4):
            yield ateles_pb2.MapDocsRequest(
                    context_id="mixed_emit",
                    map_id="map_id:%d" % i,
                    doc='{"value": %s}' % i
                )
    for idx, resp in enumerate(stub.MapDocs(gen_docs())):
        assert resp.ok
        assert resp.map_id == "map_id:%d" % idx
        row = [[[idx, None], [False, idx]], [], [[True, idx]]]
        assert json.loads(resp.result) == row


def test_map_throws(stub):
    req = ateles_pb2.CreateContextRequest(context_id="map_throws");
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="no_emit", map_funs=[
            "function(doc) {throw \"foo\";}"
        ])
    stub.AddMapFuns(req)

    def gen_docs():
        for i in range(0, 4):
            yield ateles_pb2.MapDocsRequest(
                    context_id="no_emit",
                    map_id="%d" % i,
                    doc='{"value": %s}' % i
                )
    for idx, resp in enumerate(stub.MapDocs(gen_docs())):
        assert not resp.ok
        assert resp.map_id == "%d" % grpc.StatusCode.INVALID_ARGUMENT.value[0]
        assert "Error mapping" in resp.result


def test_map_throws_error(stub):
    req = ateles_pb2.CreateContextRequest(context_id="map_throws_error");
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="map_throws_error",
        map_funs=[
            "function(doc) {do_a_thing();}"
        ])
    stub.AddMapFuns(req)

    def gen_docs():
        for i in range(0, 4):
            yield ateles_pb2.MapDocsRequest(
                    context_id="no_emit",
                    map_id="%d" % i,
                    doc='{"value": %s}' % i
                )
    for idx, resp in enumerate(stub.MapDocs(gen_docs())):
        assert not resp.ok
        assert resp.map_id == "%d" % grpc.StatusCode.INVALID_ARGUMENT.value[0]
        assert "Error mapping" in resp.result


def test_bad_map_fun(stub):
    req = ateles_pb2.CreateContextRequest(context_id="bad_js");
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="bad_js", map_funs=[
        "this is hopefully not valid JavaScript but probably is"
    ])
    try:
        stub.AddMapFuns(req)
        assert False
    except Exception, e:
        assert e.code() == grpc.StatusCode.INVALID_ARGUMENT


def test_bad_map_fun(stub):
    req = ateles_pb2.CreateContextRequest(context_id="not_a_fun");
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="not_a_fun", map_funs=[
        "var foo = 2;"
    ])
    try:
        stub.AddMapFuns(req)
        assert False
    except Exception, e:
        assert e.code() == grpc.StatusCode.INVALID_ARGUMENT
        assert "Invalid function" in e.details()


@pytest.mark.slow
def test_lots_of_map_funs(stub):
    req = ateles_pb2.CreateContextRequest(context_id="lots_of_funs");
    stub.CreateContext(req)

    long_str = "a" * 4096
    funs = [
        "function(doc) {var a = \"%s\"; emit(doc.value, null);}" % long_str
    ]
    req = ateles_pb2.AddMapFunsRequest(context_id="lots_of_funs", map_funs=funs)

    try:
        for i in range(100000):
            stub.AddMapFuns(req)
    except Exception, e:
        assert "out of memory" in e.details()
    else:
        assert False
