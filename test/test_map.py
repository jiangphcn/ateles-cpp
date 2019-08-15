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


def mkfun(id, fun):
    ret = ateles_pb2.MapFun()
    ret.id = id
    ret.fun = fun
    return ret


def test_basic_map_doc(stub):
    req = ateles_pb2.CreateContextRequest(context_id="basic_map")
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="basic_map", map_funs=[
            mkfun("1", "function(doc) {emit(doc.value, null);}")
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
        assert json.loads(resp.result) == [{'id': "1", 'result': [[idx, None]]}]


def test_no_emit(stub):
    req = ateles_pb2.CreateContextRequest(context_id="no_emit")
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="no_emit", map_funs=[
            mkfun("1", "function(doc) {return;}")
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
        assert json.loads(resp.result) == [{'id': "1", 'result': []}]


def test_multiple_emit(stub):
    req = ateles_pb2.CreateContextRequest(context_id="multiple_emits")
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="multiple_emits", map_funs=[
            mkfun("1", textwrap.dedent("""\
                    function(doc) {
                        emit(doc.value, null);
                        emit("bar", doc.value);
                    }
                    """))
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
        row = [{'id': "1", 'result': [[idx, None], ["bar", idx]]}]
        assert json.loads(resp.result) == row


def test_multiple_functions(stub):
    req = ateles_pb2.CreateContextRequest(context_id="multiple_funs")
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="multiple_funs", map_funs=[
            mkfun("1", "function(doc) {emit(doc.value, null);}"),
            mkfun("2", "function(doc) {emit(doc.value * 2, true);}"),
            mkfun("3", "function(doc) {emit(\"foo\", doc.value);}")
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
        row = [
            {'id': "1", 'result': [[idx, None]]},
            {'id': "2", 'result': [[idx * 2, True]]},
            {'id': "3", 'result': [["foo", idx]]}
        ]
        assert json.loads(resp.result) == row


def test_multiple_functions_mixed_emit(stub):
    req = ateles_pb2.CreateContextRequest(context_id="mixed_emit")
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="mixed_emit", map_funs=[
            mkfun("1", textwrap.dedent("""\
                    function(doc) {
                        emit(doc.value, null);
                        emit(false, doc.value);
                    }
                    """)),
            mkfun("2", "function(doc) {return;}"),
            mkfun("3", "function(doc) {emit(true, doc.value)}")
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
        row = [
            {'id': "1", 'result': [[idx, None], [False, idx]]},
            {'id': "2", 'result': []},
            {'id': "3", 'result': [[True, idx]]}
        ]
        assert json.loads(resp.result) == row


def test_map_throws(stub):
    req = ateles_pb2.CreateContextRequest(context_id="map_throws")
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="map_throws", map_funs=[
            mkfun("1", "function(doc) {throw \"foo\";}")
        ])
    stub.AddMapFuns(req)

    def gen_docs():
        for i in range(0, 4):
            yield ateles_pb2.MapDocsRequest(
                    context_id="map_throws",
                    map_id="%d" % i,
                    doc='{"value": %s}' % i
                )

    for idx, resp in enumerate(stub.MapDocs(gen_docs())):
        assert json.loads(resp.result) == [{'id': "1", 'error': 'foo'}]


def test_map_throws_error(stub):
    req = ateles_pb2.CreateContextRequest(context_id="map_throws_error")
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="map_throws_error",
        map_funs=[
            mkfun("1", "function(doc) {do_a_thing();}")
        ])
    stub.AddMapFuns(req)

    def gen_docs():
        for i in range(0, 4):
            yield ateles_pb2.MapDocsRequest(
                    context_id="map_throws_error",
                    map_id="map_id:%d" % i,
                    doc='{"value": %s}' % i
                )

    for idx, resp in enumerate(stub.MapDocs(gen_docs())):
        assert resp.map_id == "map_id:%d" % idx
        error = [{
            'id': "1",
            'error': 'ReferenceError: do_a_thing is not defined'}
        ]
        assert json.loads(resp.result) == error


def test_bad_map_fun(stub):
    req = ateles_pb2.CreateContextRequest(context_id="bad_js")
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="bad_js", map_funs=[
            mkfun("1", "this is hopefully not valid JavaScript but probably is")
        ])
    try:
        stub.AddMapFuns(req)
        assert False
    except Exception as e:
        assert e.code() == grpc.StatusCode.INVALID_ARGUMENT


def test_not_a_fun(stub):
    req = ateles_pb2.CreateContextRequest(context_id="not_a_fun")
    stub.CreateContext(req)

    req = ateles_pb2.AddMapFunsRequest(context_id="not_a_fun", map_funs=[
            mkfun("1", "var foo = 2;")
    ])
    try:
        stub.AddMapFuns(req)
        assert False
    except Exception as e:
        assert e.code() == grpc.StatusCode.INVALID_ARGUMENT
        assert "Invalid function" in e.details()


@pytest.mark.slow
def test_lots_of_map_funs(stub):
    req = ateles_pb2.CreateContextRequest(context_id="lots_of_funs")
    stub.CreateContext(req)

    long_str = "a" * 4096
    fun = "function(doc) {var a = \"%s\"; emit(doc.value, null);}" % long_str

    try:
        for i in range(100000):
            req = ateles_pb2.AddMapFunsRequest(
                    context_id="lots_of_funs",
                    map_funs=[
                        mkfun("%d" % i, fun)
                    ]
                )
            stub.AddMapFuns(req)
    except Exception as e:
        assert "out of memory" in e.details()
    else:
        assert False
