#!/bin/bash

# Using a single `pixi.toml` and `recipe.yaml` for all package variants is blocked on https://github.com/prefix-dev/pixi/issues/4599

for name in default asan free-threading tsan-free-threading; do
  if [[ "${name}" == *free-threading* ]]; then
    tag="cp315t"
  else
    tag="cp315"
  fi
  sed -e 's/variant: "default"/variant: "'${name}'"/g' -e 's/${{ abi_tag }}/'${tag}'/g' Tools/pixi-packages/recipe.yaml > Tools/pixi-packages/${name}/recipe.yaml
done
