## Further benchmarks
We bench against RocksDB and LMDB here.  Run and compare against GO built engines.

**For linux systems currently. Make sure you have RocksDB and LMDB installed.**

### Compile
```bash
gcc -o bench main.c -llmdb -lrocksdb -Wall
```

### Run
```bash
./db_bench --nops 1000 --lkv 32
```
