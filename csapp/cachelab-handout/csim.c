#include "cachelab.h"

#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

int hits = 0;
int misses = 0;
int evictions = 0;

void help(const char* name) {
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n"
           "Options:\n"
           "  -h         Print this help message.\n"
           "  -v         Optional verbose flag.\n"
           "  -s <num>   Number of set index bits.\n"
           "  -E <num>   Number of lines per set.\n"
           "  -b <num>   Number of block offset bits.\n"
           "  -t <file>  Trace file.\n"
           "\n"
           "Examples:\n"
           "  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n"
           "  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n",
           name, name, name);
    exit(0);
}

void parse_options(int argc, char *argv[],
    cache_config_t* config, char** tracefile,
    bool* verbose) {
    int ch;
    while ((ch = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch(ch) {
            case 'h':
                help(argv[0]);
            case 'v':
                *verbose = true;
                break;
            case 's':
                config->s = atoi(optarg);
                break;
            case 'E':
                config->E = atoi(optarg);
                break;
            case 'b':
                config->b = atoi(optarg);
                break;
            case 't':
                *tracefile = optarg;
                break;
            case '?':
                help(argv[0]);
            default:
                break;
        }
    }
}

cache_t* new_cache(cache_config_t* config) {
    cache_t* cache = malloc(sizeof(cache_t));
    cache->size = 1 << config->s;
    cache->sets = malloc(cache->size * sizeof(cache_set_t));
    for (uint64_t i = 0; i < cache->size; ++i) {
        cache_set_t* set= &cache->sets[i];
        set->size = config->E;
        set->lines = malloc(set->size * sizeof(cache_line_t));
        memset(set->lines, 0, set->size * sizeof(cache_line_t));
    }
    return cache;
}

void free_cache(cache_t* cache) {
    for (uint64_t i = 0; i < cache->size; ++i) {
        free(cache->sets[i].lines);
    }
    free(cache->sets);
    free(cache);
}

enum CacheResulType caching(
        uint64_t address,
        cache_config_t* config,
        cache_t* cache) {
    static uint64_t LRU = 1;
    uint64_t set_index = (address >> config->b) & ((1 << config->s) - 1);
    uint64_t tag_bit = (address >> (config->b + config->s));
    cache_set_t* set = &cache->sets[set_index];

    cache_line_t* found = NULL;
    for (uint64_t i = 0; i < set->size; ++i) {
        cache_line_t* line = &set->lines[i];
        if (line->valid && line->tag == tag_bit) {
            found = line;
        }
    }

    enum CacheResulType ret = Hit;
    if (found) {
        hits ++;
        ret = Hit;
    } else {
        misses ++;
        for (uint64_t i = 0; i < set->size; ++i) {
            cache_line_t* line = &set->lines[i];
            if (found == NULL || found->lru > line->lru) {
                found = line;
            }
        }
        if (found->valid) {
            ret = Eviction;
            evictions++;
        } else {
            ret = Missing;
        }

        found->tag = tag_bit;
        found->valid = true;
    }

    found->lru = LRU++;
    return ret;
}

int main(int argc, char *argv[])
{
    cache_config_t config;
    memset(&config, 0, sizeof(cache_config_t));
    char* tracefile = NULL;
    bool verbose = false;

    parse_options(argc, argv, &config, &tracefile, &verbose);
    if (config.s == 0 || config.E == 0 ||
        config.b == 0 || tracefile == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        help(argv[0]);
    }

    FILE* trace = fopen(tracefile, "r");
    if (!trace) {
        perror(tracefile);
        return EXIT_FAILURE;
    }

    cache_t* cache = new_cache(&config);

    uint64_t addr;
    char op;
    int size;
    while (fscanf(trace, " %c %lx,%d", &op, &addr, &size) != EOF) {
        if (op == 'I') {
            continue;
        }
        enum CacheResulType ret = caching(addr, &config, cache);
        if (verbose) {
            printf("%c %lx,%d", op, addr, size);
            if (ret == Missing) {
                printf(" miss");
            } else if (ret == Hit) {
                printf(" hit");
            } else if (ret == Eviction) {
                printf(" miss eviction");
            }
        }

        if (op == 'M') {
            caching(addr, &config, cache);
            if (verbose) {
                printf(" hit");
            }
        }
        if (verbose) {
            printf("\n");
        }
    }

    printSummary(hits, misses, evictions);

    return 0;
}
