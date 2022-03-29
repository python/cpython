g++ -Wl,--export-dynamic,--no-as-needed writer.cpp -lrt -Ipython3-custom/include/python3.11 -lm -ldl -lrt -lutil -pthread -Lpython3-custom/lib libpython3.11.a -o writer
g++ -Wl,--export-dynamic,--no-as-needed reader.cpp -lrt -Ipython3-custom/include/python3.11 -lm -ldl -lrt -lutil -pthread -Lpython3-custom/lib libpython3.11.a -o reader
