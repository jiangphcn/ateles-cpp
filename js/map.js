
function emitWithView (view, keys, val) {
  console.log(`${view} emitted results ${keys} : ${val}`);
};

function run (ddoc, doc) {

  Object.entries(ddoc.views).forEach(([mapName, viewFns]) => {
    const emit = emitWithView.bind(null, mapName);
    const mapFn = eval(`(${viewFns.map})`);
    mapFn(doc);
  });

};

const ddoc = {
  "_id": "_design/application",
  "_rev": "1-C1687D17",
  "views": {
      "simpleview": {
          "map": "function(doc) { emit(doc._id, 1) }",
      }
  }
}

const doc = {
  _id: "doc-id"
};


run(ddoc, doc);


/*
result = {
  docid1: {
      view1: {
          result: ok,
          view_result: [key, value]
      },
      view2: {
          result: timeout,
          error: "expected to finish within 50ms"
      }
  },
  docid2: {
      view1: {
          result: ok,
          view_result: [key, value]
      },
      view2: {
          result: timeout,
          error: "cannot compile JavaScript",
          errors: [
              "line 123: something something",
              "line 887: some warnings"
          ]
      }
  }
}
*/
