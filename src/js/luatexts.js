// LUATEXTS JavaScript module (v. 0.1.1)
// https://github.com/agladysh/luatexts/
// Copyright (c) LUATEXTS authors. Licensed under the terms of the MIT license:
// https://github.com/agladysh/luatexts/tree/master/COPYRIGHT

// -----------------------------------------------------------------------------

var LUATEXTS = (function(LUATEXTS) {

// -----------------------------------------------------------------------------

function my_typeof(v) {
	var type = typeof v;
	if (type !== "object") {
		return type;
	} else if (v === null) {
		return "null";
	} else if (v.constructor == Array) {
		return "array";
	}
  return "object";
}

function save_nil(v) {
  return '-\n';
}

function save_boolean(v) {
  return (v) ? '1\n' : '0\n';
}

// TODO: Ensure this always save number in required format
function save_number(v) {
  return 'N\n' + v.toString() + '\n';
}

// TODO: Ensure the string is in UTF-8 somehow.
function save_string(v) {
  return 'U\n' + v.length + '\n' + v + '\n';
}

function save_object(v) {
  var size = 0;
  var result = '';

  for (var k in v) {
    result += save_value(k) + save_value(v[k]);
    ++size;
  }

  return 'T\n0\n' + size + '\n' + result;
}

// Saves array as one-based table.
function save_array(v) {
  var result = 'T\n' + v.length + '\n0\n';
  for (var i = 0; i < v.length; ++i) {
    result += save_value(v[i]);
  }
  return result;
}

function not_supported(v) {
  throw new Error(
      "luatexts does not support values of type " + my_typeof(v)
    );
}

var save_by_type = {
  "undefined": save_nil,
  "null": save_nil,
  "boolean": save_boolean,
  "number": save_number,
  "string": save_string,
  "object": save_object,
  "array": save_array,
  "function": not_supported
}

function save_value(v) {
  var save_fn = save_by_type[my_typeof(v)] || not_supported;
  return save_fn(v);
}

LUATEXTS.save = function() {
  var result = arguments.length.toString() + "\n";

  for (var i = 0; i < arguments.length; ++i) {
    result += save_value(arguments[i]);
  }

  return result;
}

// Sorry, no LUATEXTS.load() yet. Patches are welcome.

// -----------------------------------------------------------------------------

  return LUATEXTS;
} (LUATEXTS || { }));

// -----------------------------------------------------------------------------
