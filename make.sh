#! /bin/bash

set -e

if ([ -z "${CLOSURE}" ]); then
  CLOSURE="$(luarocks show --rock-dir pk-core-js.google-closure-compiler)"
fi

echo "----> Going pedantic all over the source"

echo "--> c89..."
gcc -O2 -fPIC -I/usr/include/lua5.1 -c src/c/luatexts.c -o /dev/null -Isrc/c/ -Wall --pedantic -Werror --std=c89

echo "--> c99..."
gcc -O2 -fPIC -I/usr/include/lua5.1 -c src/c/luatexts.c -o /dev/null -Isrc/c/ -Wall --pedantic -Werror --std=c99

echo "--> c++98..."
gcc -xc++ -O2 -fPIC -I/usr/include/lua5.1 -c src/c/luatexts.c -o /dev/null -Isrc/c/ -Wall --pedantic -Werror --std=c++98

echo "----> Making rock"
sudo luarocks make rockspec/luatexts-scm-1.rockspec

HEADER='// LUATEXTS JavaScript module (v. 0.1.4)
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
