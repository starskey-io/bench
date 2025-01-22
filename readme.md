## Starsky Benchmarker

Build
```
go build .
```

Run
```
./bench -nops 100000 -lkv 128
```
- **nops**: Number of operations to perform
- **lkv**: Length of key and value
```

**On crappy laptop**
```
StarskyWrite benchmark: 2.672362ms
Starsky Get benchmark: 208.283µs
Starsky Delete benchmark: 2.911128ms

Pebble Write benchmark: 1.19309476s
Pebble Get benchmark: 428.02µs
Pebble Delete benchmark: 1.000259286s

BBolt Write benchmark: 4.551773ms
BBolt Get benchmark: 236.076µs
BBolt Delete benchmark: 1.136888ms
```

## Contributing
Feel free to add a key value store or make adjustments if needed.  Please submit a pull request with your changes.
If you have any questions or issues please submit an issue.