```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "commit.h"
#include "object.h"
#include "tree.h"
#include "pes.h"

/*
 * Helper: Read HEAD (parent commit)
 */
static int head_read(ObjectID *out) {
    FILE *f = fopen(".pes/HEAD", "r");
    if (!f) return -1;

    if (fscanf(f, "%40s", out->hash) != 1) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

/*
 * Helper: Update HEAD
 */
static int head_update(const ObjectID *id) {
    FILE *f = fopen(".pes/HEAD", "w");
    if (!f) return -1;

    fprintf(f, "%s\n", id->hash);
    fclose(f);
    return 0;
}

/*
 * Serialize commit object into buffer
 */
static int commit_serialize(const Commit *commit, void **data, size_t *len) {
    if (!commit || !data || !len) return -1;

    char buffer[4096];
    int offset = 0;

    // tree
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                       "tree %s\n", commit->tree.hash);

    // parent (if exists)
    if (commit->has_parent) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                           "parent %s\n", commit->parent.hash);
    }

    // author + timestamp
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                       "author %s %llu\n",
                       commit->author,
                       (unsigned long long)commit->timestamp);

    // blank line + message
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                       "\n%s\n", commit->message);

    *data = malloc(offset);
    if (!*data) return -1;

    memcpy(*data, buffer, offset);
    *len = offset;

    return 0;
}

/*
 * Create a commit
 */
int commit_create(const char *message, ObjectID *commit_id_out) {
    if (!message || !commit_id_out) return -1;

    // 1. Create tree from index
    ObjectID tree_id;
    if (tree_from_index(&tree_id) != 0) {
        return -1;
    }

    // 2. Initialize commit struct
    Commit commit;
    memset(&commit, 0, sizeof(Commit));

    commit.tree = tree_id;

    // 3. Read parent (HEAD)
    ObjectID parent_id;
    if (head_read(&parent_id) == 0) {
        commit.parent = parent_id;
        commit.has_parent = 1;
    } else {
        commit.has_parent = 0; // first commit
    }

    // 4. Set author
    snprintf(commit.author, sizeof(commit.author),
             "%s", pes_author());

    // 5. Set timestamp
    commit.timestamp = (uint64_t)time(NULL);

    // 6. Set message
    snprintf(commit.message, sizeof(commit.message),
             "%s", message);

    // 7. Serialize commit
    void *data = NULL;
    size_t len = 0;
    if (commit_serialize(&commit, &data, &len) != 0) {
        return -1;
    }

    // 8. Write commit object
    ObjectID commit_id;
    if (object_write(OBJ_COMMIT, data, len, &commit_id) != 0) {
        free(data);
        return -1;
    }

    free(data);

    // 9. Update HEAD
    if (head_update(&commit_id) != 0) {
        return -1;
    }

    // 10. Return commit ID
    *commit_id_out = commit_id;

    return 0;
}
```
