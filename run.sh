set -e -x
git clean -fdx
./configure --enable-optimizations PROFILE_TASK="profile_task.py"
make
./python -m venv env
env/bin/python -m pip install pyperf
env/bin/python bench.py -o bench.json -v
