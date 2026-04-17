```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"
#include "object.h"
#include "pes.h"

#define INDEX_FILE ".pes/index"

/*
 * Load index from file
 */
int index_load(Index *idx) {
    if (!idx) return -1;

    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) {
        idx->count = 0; // empty index if file doesn't exist
        return 0;
    }

    idx->count = 0;

    while (!feof(f)) {
        IndexEntry *e = &idx->entries[idx->count];

        if (fscanf(f, "%255s %40s\n", e->path, e->oid.hash) == 2) {
            idx->count++;
        }
    }

    fclose(f);
    return 0;
}

/*
 * Save index to file
 */
int index_save(const Index *idx) {
    if (!idx) return -1;

    FILE *f = fopen(INDEX_FILE, "w");
    if (!f) return -1;

    for (size_t i = 0; i < idx->count; i++) {
        fprintf(f, "%s %s\n",
                idx->entries[i].path,
                idx->entries[i].oid.hash);
    }

    fclose(f);
    return 0;
}

/*
 * Add file to index
 */
int index_add(Index *idx, const char *path) {
    if (!idx || !path) return -1;

    // 1. Read file content
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    void *buffer = malloc(size);
    if (!buffer) {
        fclose(f);
        return -1;
    }

    fread(buffer, 1, size, f);
    fclose(f);

    // 2. Write blob object
    ObjectID oid;
    if (object_write(OBJ_BLOB, buffer, size, &oid) != 0) {
        free(buffer);
        return -1;
    }

    free(buffer);

    // 3. Check if file already exists → update
    for (size_t i = 0; i < idx->count; i++) {
        if (strcmp(idx->entries[i].path, path) == 0) {
            idx->entries[i].oid = oid;
            return index_save(idx);
        }
    }

    // 4. Add new entry
    IndexEntry *e = &idx->entries[idx->count++];
    strncpy(e->path, path, sizeof(e->path));
    e->oid = oid;

    // 5. Save index
    return index_save(idx);
}
```
