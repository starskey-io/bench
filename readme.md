## Starsky Benchmarker

Build
```
go build .
```

Run
```
./bench -nops 100000 -lkv 128
```

- **nops** Number of operations to perform
- **lkv** Length of key and value

**On crappy 4 year old system** run on your server or local machine for better results and on an SSD!

10000 ops with 32 byte key and value
```
Starsky Write benchmark: 53.308074ms
Starsky Get benchmark: 4.811411ms
Starsky Delete benchmark: 28.883631ms

Pebble Write benchmark: 10.784630479s
Pebble Get benchmark: 7.294086ms
Pebble Delete benchmark: 11.603169612s

BBolt Write benchmark: 67.20567ms
BBolt Get benchmark: 4.564291ms
BBolt Delete benchmark: 5.458657ms

Badger Write benchmark: 6.605586446s
Badger Get benchmark: 12.766663ms
Badger Delete benchmark: 6.023307227s

LMDB Write benchmark: 3.741000ms
LMDB Get benchmark: 1.996000ms
LMDB Delete benchmark: 3.403000ms

RocksDB Write benchmark: 286.912000ms
RocksDB Get benchmark: 10.144000ms
RocksDB Delete benchmark: 283.054000ms

```

## Contributing
Feel free to add a key value store or make adjustments if needed.  Please submit a pull request with your changes.
If you have any questions or issues please submit an issue.