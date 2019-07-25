
var map_funs = [];
var results = [];


function emit(key, value) {
  results.push([key, value]);
}


function add_fun(source) {
  map_funs.push(eval(source));
}


function map_doc(doc) {
  var output = [];
  for(var i = 0; i < map_funs.length; i++) {
    results = [];
    map_funs[i](doc)
    output.push(results)
  }
  return JSON.stringify(output);
}
