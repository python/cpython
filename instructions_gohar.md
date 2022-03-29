## Build CPython
```
LIBS="-lrt" ./configure --prefix=$HOME/cpython/python3-custom
make
sudo make install
```

## Build Toy Examples
```
./build.sh
```

## Run Toy Examples
### Writer
```
./writer helloworld
```

Note the output from `malloc`.
e.g., `malloc 0x7fde30f32008, 0x7fde30f32000, 85788330, 62`

### Reader
Using the output from *Writer*:
```
./reader 85788330 62
```
