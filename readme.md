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

1000 ops with 128 byte key and value
```
Starsky Write benchmark: 3.198341ms
Starsky Get benchmark: 184.96µs
Starsky Delete benchmark: 2.369965ms

Pebble Write benchmark: 1.019500292s
Pebble Get benchmark: 408.216µs
Pebble Delete benchmark: 1.066333699s

BBolt Write benchmark: 3.941174ms
BBolt Get benchmark: 242.924µs
BBolt Delete benchmark: 1.134009ms

Badger Write benchmark: 527.538019ms
Badger Get benchmark: 1.49239ms
Badger Delete benchmark: 521.008806ms
```

## Contributing
Feel free to add a key value store or make adjustments if needed.  Please submit a pull request with your changes.
If you have any questions or issues please submit an issue.