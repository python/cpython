Issues #26289 and #26315: Optimize floor and modulo division for
single-digit longs.  Microbenchmarks show 2-2.5x improvement.  Built-in
'divmod' function is now also ~10% faster.