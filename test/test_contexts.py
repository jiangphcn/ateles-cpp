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

import grpc
import pytest

import ateles_pb2
import ateles_pb2_grpc

import tutil


@pytest.fixture()
def stub():
    return tutil.connect(reset=True)


def test_create_context(stub):
    req = ateles_pb2.CreateContextRequest(context_id="foo");
    stub.CreateContext(req)


def test_create_existing(stub):
    req = ateles_pb2.CreateContextRequest(context_id="foo");
    stub.CreateContext(req)

    req = ateles_pb2.CreateContextRequest(context_id="foo");
    try:
        stub.CreateContext(req)
    except Exception, e:
        assert e.code() == grpc.StatusCode.ALREADY_EXISTS


@pytest.mark.slow
def test_create_lots_of_contexts(stub):
    for i in range(100):
        req = ateles_pb2.CreateContextRequest(context_id="foo:%d" % i)
        stub.CreateContext(req)


def test_add_map_funs_missing_context(stub):
    map_fun = ateles_pb2.MapFun()
    map_fun.id = "bad-id"
    map_fun.fun = "function(doc) {throw \"foo\";}"
    req = ateles_pb2.AddMapFunsRequest(context_id="no_emit", map_funs=[
            map_fun
        ])
    try:
        stub.AddMapFuns(req)
    except Exception, e:
        assert e.code() == grpc.StatusCode.NOT_FOUND


def test_map_docs_missing_context(stub):
    def gen_docs():
        for i in range(0, 4):
            yield ateles_pb2.MapDocsRequest(
                    context_id="basic_map",
                    map_id="map_id:%d" % i,
                    doc='{"value": %s}' % i
                )
    count = 0
    try:
        for resp in stub.MapDocs(gen_docs()):
            count += 1
    except Exception, e:
        assert e.code() == grpc.StatusCode.NOT_FOUND
    # This is asserting that the request died before
    # processing anything:
    assert count == 0