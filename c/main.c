/* Package starskey
 *
 * (C) Copyright Starskey
 *
 * Original Author: Alex Gaetano Padula
 *
 * Licensed under the Mozilla Public License, v. 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.mozilla.org/en-US/MPL/2.0/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <lmdb.h>
#include <rocksdb/c.h>
#include <limits.h>

/* configuration */
static int n_ops = 1000;    /* number of operations */
static int kv_len = 32;     /* length of key-value pairs */

/* structure to hold key-value pairs */
struct kv_pair {
    char *key;
    char *value;
};

/* function declarations */
char *generate_random_string();
struct kv_pair *generate_random_pairs();
void free_pairs();
void bench_lmdb();
void bench_rocksdb();
void print_usage();

/* helper function to generate random string */
char *
generate_random_string(length)
    int length;
{
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char *str;
    int i, index;

    str = malloc(length + 1);
    for (i = 0; i < length; i++) {
        index = rand() % (sizeof(charset) - 1);
        str[i] = charset[index];
    }
    str[length] = '\0';
    return str;
}

/* generate random key-value pairs */
struct kv_pair *
generate_random_pairs(num_items, length)
    int num_items;
    int length;
{
    struct kv_pair *pairs;
    char temp[32];
    int i;

    pairs = malloc(sizeof(struct kv_pair) * num_items);
    for (i = 0; i < num_items; i++) {
        sprintf(temp, "%d%s", rand() % 1000000, generate_random_string(length));
        pairs[i].key = strdup(temp);

        sprintf(temp, "%d%s", rand() % 1000000, generate_random_string(length));
        pairs[i].value = strdup(temp);
    }
    return pairs;
}

/* free key-value pairs */
void
free_pairs(pairs, num_items)
    struct kv_pair *pairs;
    int num_items;
{
    int i;

    for (i = 0; i < num_items; i++) {
        free(pairs[i].key);
        free(pairs[i].value);
    }
    free(pairs);
}

/* benchmark lmdb */
void
bench_lmdb()
{
    struct kv_pair *pairs;
    MDB_env *env;
    MDB_dbi dbi;
    MDB_txn *txn;
    int rc;
    clock_t start;
    double duration;
    int i;

    printf("Running LMDB benchmark...\n");

    /* generate test data */
    pairs = generate_random_pairs(n_ops, kv_len);

    mkdir("lmdb_bench", 0755);

    rc = mdb_env_create(&env);
    if (rc) {
        fprintf(stderr, "mdb_env_create failed, error %d\n", rc);
        return;
    }

    rc = mdb_env_set_mapsize(env, 10485760); /* 10MB */
    if (rc) {
        fprintf(stderr, "mdb_env_set_mapsize failed, error %d\n", rc);
        mdb_env_close(env);
        return;
    }

    rc = mdb_env_open(env, "lmdb_bench", 0, 0644);
    if (rc) {
        fprintf(stderr, "mdb_env_open failed, error %d\n", rc);
        mdb_env_close(env);
        return;
    }

    /* benchmark write */
    start = clock();
    rc = mdb_txn_begin(env, NULL, 0, &txn);
    rc = mdb_dbi_open(txn, NULL, 0, &dbi);

    for (i = 0; i < n_ops; i++) {
        MDB_val key = {strlen(pairs[i].key), pairs[i].key};
        MDB_val value = {strlen(pairs[i].value), pairs[i].value};
        rc = mdb_put(txn, dbi, &key, &value, 0);
        if (rc) {
            fprintf(stderr, "mdb_put failed, error %d\n", rc);
            break;
        }
    }

    rc = mdb_txn_commit(txn);
    duration = ((double)(clock() - start)) / CLOCKS_PER_SEC;
    if (duration >= 1.0)
        printf("LMDB Write benchmark: %.9fs\n", duration);
    else if (duration >= 0.001)
        printf("LMDB Write benchmark: %.6fms\n", duration * 1000);
    else
        printf("LMDB Write benchmark: %.3fµs\n", duration * 1000000);

    /* benchmark read */
    start = clock();
    rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);

    for (i = 0; i < n_ops; i++) {
        MDB_val key = {strlen(pairs[i].key), pairs[i].key};
        MDB_val value;
        rc = mdb_get(txn, dbi, &key, &value);
        if (rc) {
            fprintf(stderr, "mdb_get failed, error %d\n", rc);
            break;
        }
    }

    mdb_txn_abort(txn);
    duration = ((double)(clock() - start)) / CLOCKS_PER_SEC;
    if (duration >= 1.0)
        printf("LMDB Get benchmark: %.9fs\n", duration);
    else if (duration >= 0.001)
        printf("LMDB Get benchmark: %.6fms\n", duration * 1000);
    else
        printf("LMDB Get benchmark: %.3fµs\n", duration * 1000000);

    /* benchmark delete */
    start = clock();
    rc = mdb_txn_begin(env, NULL, 0, &txn);

    for (i = 0; i < n_ops; i++) {
        MDB_val key = {strlen(pairs[i].key), pairs[i].key};
        rc = mdb_del(txn, dbi, &key, NULL);
        if (rc) {
            fprintf(stderr, "mdb_del failed, error %d\n", rc);
            break;
        }
    }

    rc = mdb_txn_commit(txn);
    duration = ((double)(clock() - start)) / CLOCKS_PER_SEC;
    if (duration >= 1.0)
        printf("LMDB Delete benchmark: %.9fs\n", duration);
    else if (duration >= 0.001)
        printf("LMDB Delete benchmark: %.6fms\n", duration * 1000);
    else
        printf("LMDB Delete benchmark: %.3fµs\n", duration * 1000000);

    /* cleanup */
    mdb_dbi_close(env, dbi);
    mdb_env_close(env);
    system("rm -rf lmdb_bench");
    free_pairs(pairs, n_ops);
    printf("\n");
}

/* benchmark rocksdb */
void
bench_rocksdb()
{
    struct kv_pair *pairs;
    rocksdb_t *db;
    rocksdb_options_t *options;
    rocksdb_writeoptions_t *writeoptions;
    rocksdb_readoptions_t *readoptions;
    char *err;
    clock_t start;
    double duration;
    int i;

    printf("Running RocksDB benchmark...\n");

    /* generate test data */
    pairs = generate_random_pairs(n_ops, kv_len);

    /* initialize */
    options = rocksdb_options_create();
    rocksdb_options_set_create_if_missing(options, 1);
    rocksdb_options_set_use_fsync(options, 1);

    err = NULL;
    db = rocksdb_open(options, "rocksdb_bench", &err);
    if (err) {
        fprintf(stderr, "Failed to open RocksDB: %s\n", err);
        free(err);
        return;
    }

    writeoptions = rocksdb_writeoptions_create();
    rocksdb_writeoptions_set_sync(writeoptions, 1);
    readoptions = rocksdb_readoptions_create();

    /* benchmark write */
    start = clock();
    for (i = 0; i < n_ops; i++) {
        rocksdb_put(db, writeoptions,
                    pairs[i].key, strlen(pairs[i].key),
                    pairs[i].value, strlen(pairs[i].value),
                    &err);
        if (err) {
            fprintf(stderr, "RocksDB put failed: %s\n", err);
            free(err);
            break;
        }
    }
    duration = ((double)(clock() - start)) / CLOCKS_PER_SEC;
    if (duration >= 1.0)
        printf("RocksDB Write benchmark: %.9fs\n", duration);
    else if (duration >= 0.001)
        printf("RocksDB Write benchmark: %.6fms\n", duration * 1000);
    else
        printf("RocksDB Write benchmark: %.3fµs\n", duration * 1000000);

    /* benchmark read */
    start = clock();
    for (i = 0; i < n_ops; i++) {
        size_t len;
        char *value = rocksdb_get(db, readoptions,
                                 pairs[i].key, strlen(pairs[i].key),
                                 &len, &err);
        if (err) {
            fprintf(stderr, "RocksDB get failed: %s\n", err);
            free(err);
            break;
        }
        free(value);
    }
    duration = ((double)(clock() - start)) / CLOCKS_PER_SEC;
    if (duration >= 1.0)
        printf("RocksDB Get benchmark: %.9fs\n", duration);
    else if (duration >= 0.001)
        printf("RocksDB Get benchmark: %.6fms\n", duration * 1000);
    else
        printf("RocksDB Get benchmark: %.3fµs\n", duration * 1000000);

    /* benchmark delete */
    start = clock();
    for (i = 0; i < n_ops; i++) {
        rocksdb_delete(db, writeoptions,
                      pairs[i].key, strlen(pairs[i].key),
                      &err);
        if (err) {
            fprintf(stderr, "RocksDB delete failed: %s\n", err);
            free(err);
            break;
        }
    }
    duration = ((double)(clock() - start)) / CLOCKS_PER_SEC;
    if (duration >= 1.0)
        printf("RocksDB Delete benchmark: %.9fs\n", duration);
    else if (duration >= 0.001)
        printf("RocksDB Delete benchmark: %.6fms\n", duration * 1000);
    else
        printf("RocksDB Delete benchmark: %.3fµs\n", duration * 1000000);

    /* cleanup */
    rocksdb_close(db);
    rocksdb_options_destroy(options);
    rocksdb_readoptions_destroy(readoptions);
    rocksdb_writeoptions_destroy(writeoptions);
    system("rm -rf rocksdb_bench");
    free_pairs(pairs, n_ops);
    printf("\n");
}

void
print_usage(program_name)
    const char *program_name;
{
    fprintf(stderr, "Usage: %s [options]\n", program_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --nops N    Number of operations (default 1000)\n");
    fprintf(stderr, "  --lkv N     Length of key-value pairs (default 32)\n");
    fprintf(stderr, "  --help      Display this help message\n");
}

int
main(argc, argv)
    int argc;
    char *argv[];
{
    int i;
    char *endptr;
    long val;


    printf("Arguments received:\n");
    for (i = 0; i < argc; i++)
        printf("argv[%d]: %s\n", i, argv[i]);

    /* parse command line arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }

        if (strcmp(argv[i], "--nops") == 0) {
            if (i + 1 < argc) {
                val = strtol(argv[i + 1], &endptr, 10);
                if (*endptr != '\0' || val <= 0 || val > INT_MAX) {
                    fprintf(stderr, "Error: Invalid value for --nops: %s\n", argv[i + 1]);
                    return 1;
                }
                n_ops = (int)val;
                printf("Setting n_ops to: %d\n", n_ops);
                i++;
            } else {
                fprintf(stderr, "Error: Missing value for --nops\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--lkv") == 0) {
            if (i + 1 < argc) {
                val = strtol(argv[i + 1], &endptr, 10);
                if (*endptr != '\0' || val <= 0 || val > INT_MAX) {
                    fprintf(stderr, "Error: Invalid value for --lkv: %s\n", argv[i + 1]);
                    return 1;
                }
                kv_len = (int)val;
                printf("Setting kv_len to: %d\n", kv_len);
                i++;
            } else {
                fprintf(stderr, "Error: Missing value for --lkv\n");
                return 1;
            }
        } else {
            fprintf(stderr, "Error: Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    printf("Running benchmarks with %d operations and key-value length of %d\n\n",
           n_ops, kv_len);

    /* initialize random seed */
    srand(time(NULL));

    /* run benchmarks */
    bench_lmdb();
    bench_rocksdb();

    return 0;
}