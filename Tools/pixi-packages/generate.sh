#!/bin/bash

# Using a single `pixi.toml` and `recipe.yaml` for all package variants is blocked on https://github.com/prefix-dev/pixi/issues/4599

for name in asan free-threading tsan-free-threading; do
  cp Tools/pixi-packages/default/recipe.yaml Tools/pixi-packages/${name}/recipe.yaml
done
