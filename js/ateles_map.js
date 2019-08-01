
var lib = {};
var map_funs = [];
var results = [];


function emit(key, value) {
  results.push([key, value]);
}

function set_lib(source) {
  lib = JSON.parse(source);
}

function add_fun(source) {
  var fun = eval(source);
  if(typeof fun === "function") {
    map_funs.push(fun)
  } else {
    throw "Invalid function"
  }
}

function map_doc(doc_str) {
  var doc = JSON.parse(doc_str);
  var output = [];
  for(var i = 0; i < map_funs.length; i++) {
    results = [];
    map_funs[i](doc)
    output.push(results)
  }
  return JSON.stringify(output);
}
