var esprima = require ('esprima');
var escodegen = require ('escodegen');

var str = "Set.prototype.member = function (el) { return hasOwn.call(this.set, el); };"

console.log (escodegen.generate(esprima.parse(str)));

