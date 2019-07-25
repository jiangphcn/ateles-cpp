// Based on the normalizeFunction which can be
// found here:
//
//  https://github.com/dmunch/couch-chakra/blob/master/js/normalizeFunction.js

function rewrite_anon_fun(fun) {
	var ast = esprima.parse(fun);
	var idx = ast.body.length - 1;
  var decl = {};

  // Search for the first FunctionDeclaration beginning from the end
  do {
    decl = ast.body[idx--];
  } while(idx >= 0 && decl.type != "FunctionDeclaration")
  idx++;

  // If we have a function declaration without an Id, wrap it
  // in an ExpressionStatement and change it into
  // a FuntionExpression
	if(decl.type == "FunctionDeclaration" && decl.id == null){
    decl.type = "FunctionExpression";
    ast.body[idx] = {
       "type": "ExpressionStatement",
       "expression": decl
    };
	}

  // Generate source from the rewritten AST
	return escodegen.generate(ast);
}