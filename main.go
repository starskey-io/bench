// Package starskey
//
// (C) Copyright Starskey
//
// Original Author: Alex Gaetano Padula
//
// Licensed under the Mozilla Public License, v. 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.mozilla.org/en-US/MPL/2.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package main

import (
	"flag"
	"fmt"
	"github.com/cockroachdb/pebble"
	"github.com/dgraph-io/badger/v4"
	"github.com/starskey-io/starskey"
	bolt "go.etcd.io/bbolt"
	"log"
	"math/rand"
	"os"
	"time"
)

var (
	nOps  int
	kvLen int
)

// Random reads, writes, deletes.
// Synced... so we can compare the performance of the storage engines.
// Starsky syncs to disk automatically, thus the others should sync as well.

func generateRandomString(n int) string {
	var letters = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
	b := make([]rune, n)
	for i := range b {
		b[i] = letters[rand.Intn(len(letters))]
	}
	return string(b)

}

func generateRandomPairs(numItems int, length int) []struct{ key, value string } {
	pairs := make([]struct{ key, value string }, numItems)
	for i := 0; i < numItems; i++ {
		pairs[i] = struct{ key, value string }{
			key:   fmt.Sprintf("%d%s", rand.Intn(1000000), generateRandomString(length)),
			value: fmt.Sprintf("%d%s", rand.Intn(1000000), generateRandomString(length)),
		}
	}
	return pairs
}

func BenchStarskey() {
	var dbPath = "starskey_bench"
	defer os.RemoveAll(dbPath)

	pairs := generateRandomPairs(nOps, kvLen)

	// Open Starskey database
	db, err := starskey.Open(&starskey.Config{
		Permission:        0755,
		Directory:         dbPath,
		FlushThreshold:    (1024 * 1024) * 64,
		MaxLevel:          3,
		SizeFactor:        10,
		BloomFilter:       false,
		Logging:           true,
		Compression:       false,
		CompressionOption: starskey.NoCompression,
	})
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	// Benchmark Write
	start := time.Now()
	for _, pair := range pairs {
		err = db.Put([]byte(pair.key), []byte(pair.value))
		if err != nil {
			log.Fatal(err)
		}
	}
	fmt.Printf("Starsky Write benchmark: %v\n", time.Since(start))

	// Benchmark Get
	start = time.Now()
	for _, pair := range pairs {
		_, err := db.Get([]byte(pair.key))
		if err != nil {
			log.Fatal(err)
		}
	}
	fmt.Printf("Starsky Get benchmark: %v\n", time.Since(start))

	// Benchmark Delete
	start = time.Now()
	for _, pair := range pairs {
		err = db.Delete([]byte(pair.key))
		if err != nil {
			log.Fatal(err)
		}
	}
	fmt.Printf("Starsky Delete benchmark: %v\n", time.Since(start))
}

func BenchPebble() {
	var dbPath = "pebble_bench"
	defer os.RemoveAll(dbPath)

	pairs := generateRandomPairs(nOps, kvLen)

	// Open Pebble database
	db, err := pebble.Open(dbPath, &pebble.Options{DisableWAL: false})
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	// Benchmark Write
	start := time.Now()
	for _, pair := range pairs {
		err = db.Set([]byte(pair.key), []byte(pair.value), pebble.Sync)
		if err != nil {
			log.Fatal(err)
		}
	}
	fmt.Printf("Pebble Write benchmark: %v\n", time.Since(start))

	// Benchmark Get
	start = time.Now()
	for _, pair := range pairs {
		_, closer, err := db.Get([]byte(pair.key))
		if err != nil {
			log.Fatal(err)
		}
		closer.Close()
	}
	fmt.Printf("Pebble Get benchmark: %v\n", time.Since(start))

	// Benchmark Delete
	start = time.Now()
	for _, pair := range pairs {
		err = db.Delete([]byte(pair.key), pebble.Sync)
		if err != nil {
			log.Fatal(err)
		}
	}
	fmt.Printf("Pebble Delete benchmark: %v\n", time.Since(start))
}

func BenchBBolt() {
	var dbPath = "bench.db"
	var bucket = "bench"
	defer os.Remove(dbPath)

	pairs := generateRandomPairs(nOps, kvLen)

	db, err := bolt.Open(dbPath, 0600, nil)
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	// Benchmark Write
	start := time.Now()
	err = db.Update(func(tx *bolt.Tx) error {
		b, err := tx.CreateBucketIfNotExists([]byte(bucket))
		if err != nil {
			return err
		}
		for _, pair := range pairs {
			err = b.Put([]byte(pair.key), []byte(pair.value))
			if err != nil {
				return err
			}
		}
		return nil
	})
	if err != nil {
		log.Fatal(err)
	}
	fmt.Printf("BBolt Write benchmark: %v\n", time.Since(start))

	// Benchmark Get
	start = time.Now()
	err = db.View(func(tx *bolt.Tx) error {
		b := tx.Bucket([]byte(bucket))
		if b == nil {
			return fmt.Errorf("bucket %s not found", bucket)
		}
		for _, pair := range pairs {
			_ = b.Get([]byte(pair.key))
		}
		return nil
	})
	if err != nil {
		log.Fatal(err)
	}
	fmt.Printf("BBolt Get benchmark: %v\n", time.Since(start))

	// Benchmark Delete
	start = time.Now()
	err = db.Update(func(tx *bolt.Tx) error {
		b := tx.Bucket([]byte(bucket))
		if b == nil {
			return fmt.Errorf("bucket %s not found", bucket)
		}
		for _, pair := range pairs {
			err = b.Delete([]byte(pair.key))
			if err != nil {
				return err
			}
		}
		return nil
	})
	if err != nil {
		log.Fatal(err)
	}
	fmt.Printf("BBolt Delete benchmark: %v\n", time.Since(start))
}

func BenchBadger() {
	var dbPath = "badger_bench"
	defer os.RemoveAll(dbPath)

	pairs := generateRandomPairs(nOps, kvLen)

	opts := badger.DefaultOptions(dbPath)
	opts.Logger = nil      // Disable logging for benchmark
	opts.SyncWrites = true // Ensure writes are synced to disk
	db, err := badger.Open(opts)
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	// Benchmark Write
	start := time.Now()
	for _, pair := range pairs {
		err = db.Update(func(txn *badger.Txn) error {
			return txn.Set([]byte(pair.key), []byte(pair.value))
		})
		if err != nil {
			log.Fatal(err)
		}
	}
	fmt.Printf("Badger Write benchmark: %v\n", time.Since(start))

	// Benchmark Get
	start = time.Now()
	for _, pair := range pairs {
		err = db.View(func(txn *badger.Txn) error {
			item, err := txn.Get([]byte(pair.key))
			if err != nil {
				return err
			}
			return item.Value(func(val []byte) error {
				return nil
			})
		})
		if err != nil {
			log.Fatal(err)
		}
	}
	fmt.Printf("Badger Get benchmark: %v\n", time.Since(start))

	// Benchmark Delete
	start = time.Now()
	for _, pair := range pairs {
		err = db.Update(func(txn *badger.Txn) error {
			return txn.Delete([]byte(pair.key))
		})
		if err != nil {
			log.Fatal(err)
		}
	}
	fmt.Printf("Badger Delete benchmark: %v\n", time.Since(start))
}

func main() {
	flag.IntVar(&nOps, "nops", 1000, "number of operations")
	flag.IntVar(&kvLen, "lkv", 32, "length of key-value pairs")
	flag.Parse()

	BenchStarskey()
	fmt.Println()
	BenchPebble()
	fmt.Println()
	BenchBBolt()
	fmt.Println()
	BenchBadger()
}
