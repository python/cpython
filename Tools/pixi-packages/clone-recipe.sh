#!/bin/bash

# Please always only modify default/recipe.yaml and default/pixi.toml and then run this
# script to propagate the changes to the other variants.
set -o errexit
cd "$(dirname "$0")"

for variant in asan freethreading tsan-freethreading; do
    cp -av default/recipe.yaml default/pixi.toml ${variant}/
done
