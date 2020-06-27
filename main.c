#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct _memory_region_t {
  uintptr_t start;
  uintptr_t end;
  char *pathname; // Optional
  struct _memory_region_t *next;
} memory_region_t;

typedef memory_region_t *memory_regions_t;

static void usage(const char *execname) {
  printf("Usage: %s [-p pid] [-h]\n", execname);
  exit(EXIT_SUCCESS);
}

#define L(_msg, ...)                          \
  do {                                        \
    printf("[+] " #_msg "\n", ##__VA_ARGS__); \
  } while (0)

#define F(_ptr, _msg, ...)                                      \
  do {                                                          \
    if (!_ptr) {                                                \
      fprintf(stderr, "[-] Error: " #_msg "\n", ##__VA_ARGS__); \
      exit(EXIT_FAILURE);                                       \
    }                                                           \
  } while (0)

static memory_regions_t get_memory_map(pid_t pid, unsigned *count) {
  assert(pid && count && "Invalid arguments to get_memory_map");
  char path[32];
  snprintf(path, sizeof(path), "/proc/%u/maps", pid);
  L("Targeting process %d", pid);

  FILE *fp = fopen(path, "r");
  F(fp, "Error opening memory map %s: %s\n", path, strerror(errno));

  // Read and parse each line in the maps file for 'pid'.
  *count = 0;
  char line[128];
  memory_regions_t head = NULL;
  while (fgets(line, sizeof(line), fp)) {
    memory_region_t *mm = calloc(1, sizeof(memory_region_t));
    if (!mm) {
      fprintf(stderr, "Error allocating memory for a map entry.\n");
      exit(EXIT_FAILURE);
    }

    // Read in the start and end memory range value, and the permission field.
    char *range = strtok(line, " ");
    F(range, "Obtaining range value.");
    char *perm = strtok(NULL, " ");
    F(perm, "Obtaining the permission field.");
   
    // Skip if this is not writable.
    if (perm[0] != 'w') {
      free(mm);
      continue;
    }
  
    // Convert the range to integer values.   
    char *sep = strchr(range, '-');
    F(sep, "Obtaining range separator character.");
    range[sep - range] = '\0';
    mm->start = strtoll(range, NULL, 16);
    mm->end = strtoll(sep + 1, NULL, 16);
    F(mm->start, "Obtaining the range start value.");
    F(mm->end, "Obtaining the range end value.");

    // Skip offset,dev,inode and get the pathname field (last field).
    strtok(NULL, " "); // Offset  
    strtok(NULL, " "); // Device
    strtok(NULL, " "); // Inode
    char *path = strtok(NULL, " ");
    if (path)
      mm->pathname = strdup(path);
    mm->next = head;
    head = mm;
    ++*count;
  }

  fclose(fp);
  return head;
}

int main(int argc, char **argv) {
  int opt = 0;
  pid_t pid = 0;
  while ((opt = getopt(argc, argv, "p:h")) != -1) {
    switch (opt) {
      case 'p':
        pid = atoi(optarg);
        break;
      case 'h':
      default:
        usage(argv[0]);
    }
  }

  if (pid == 0) {
    fprintf(stderr, "Invalid arguments, -p must be specified.");
    return -1;
  }

#ifdef TEST_DEBUG
  pid = getpid();
#endif

  unsigned count = 0;
  memory_regions_t regions = get_memory_map(pid, &count);
  L("Found %u candidate memory regions to write to.", count);
  for (const memory_region_t *mm = regions; mm; mm = mm->next) {
  }
  return 0;
}
