#! /bin/bash

set -e

if ([ -z "${CLOSURE}" ]); then
  CLOSURE="$(luarocks show --rock-dir pk-core-js.google-closure-compiler)"
fi

echo "----> Making rock"
sudo luarocks make rockspec/luatexts-scm-1.rockspec

HEADER='// LUATEXTS JavaScript module (v. 0.1)
// https://github.com/agladysh/luatexts/
// Copyright (c) LUATEXTS authors. Licensed under the terms of the MIT license:
// https://github.com/agladysh/luatexts/tree/master/COPYRIGHT
%output%
'

echo "----> Minifying js"
java -jar \
  "${CLOSURE}/tools/closure-compiler/compiler.jar" \
  --charset=UTF-8 \
  --compilation_level=SIMPLE_OPTIMIZATIONS \
  --js=src/js/luatexts.js \
  --js_output_file=src/js/luatexts.min.js \
  --output_wrapper "${HEADER}"

echo "----> OK"
