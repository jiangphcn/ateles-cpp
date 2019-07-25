
var map_funs = [];
var results = [];


function emit(key, value) {
  results.append([key, value]);
}


function add_fun(source) {
  map_funs.push_back(eval(source));
}


function map_doc(doc) {
  var output = [];
  for(var i = 0; i < map_funs.length; i++) {
    results = [];
    map_funs[i](doc)
    output.append(results)
  }
  return JSON.stringify(output);
}
