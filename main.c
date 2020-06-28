#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

typedef struct _memory_region_t {
  uintptr_t start;
  uintptr_t end;
  char permissions[4];
  char *pathname;  // Optional
  struct _memory_region_t *next;
} memory_region_t;

typedef memory_region_t *memory_regions_t;

static void usage(const char *execname) {
  printf(
      "Usage: %s [-p pid] [-n num] [-h]\n"
      "  -p <pid>: Process ID of the excutable to attack.\n"
      "  -n <num>: Number of times to attack the process.\n"
      "  -h:       This help messages.\n",
      execname);
  exit(EXIT_SUCCESS);
}

#define L(_msg, ...)                         \
  do {                                       \
    printf("[+] " _msg "\n", ##__VA_ARGS__); \
  } while (0)

#define F(_ptr, _msg, ...)                                     \
  do {                                                         \
    if (!_ptr) {                                               \
      fprintf(stderr, "[-] Error: " _msg "\n", ##__VA_ARGS__); \
      exit(EXIT_FAILURE);                                      \
    }                                                          \
  } while (0)

#define FE(_expr, _msg, ...)                                        \
  do {                                                              \
    if ((_expr)) F(0, _msg ": %s", ##__VA_ARGS__, strerror(errno)); \
  } while (0)

static const memory_region_t *get_region(const memory_regions_t regions,
                                         int idx) {
  const memory_region_t *mm = regions;
  for (int i = 0; mm && i < idx; ++i, mm = mm->next)
    ;
  return mm;
}

static uintptr_t get_random_address(const memory_region_t *region) {
  assert(region && "No region was passed to get_random_address");
  const uintptr_t n = region->end - region->start;
  return region->start + (rand() % n);
}

static int n_read_errs, n_write_errs;

static void attack(pid_t pid, const memory_regions_t regions, unsigned count,
                   unsigned iter, unsigned n_iters) {
  const int n = rand() % count;
  const int bit = rand() % 8;
  const memory_region_t *region = get_region(regions, n);
  assert(region && "Failed to obtain a region.");
  uintptr_t addr = get_random_address(region);
  printf(
      "[%d/%d] Flipping bit %d at address %p in memory region index %d. (%s)",
      iter + 1, n_iters, bit, (void *)addr, n,
      region->pathname ? region->pathname : "unnamed");

  // Interrupt the process.
  bool found_errs = false;
  int err = ptrace(PTRACE_INTERRUPT, pid, NULL, NULL);
  FE(err, "Unable to interrupt the target process.");

  // Read a byte to manipulate.
  long data = ptrace(PTRACE_PEEKDATA, pid, addr, 0);
  n_read_errs += !!errno;
  found_errs = !!errno;

  // Write that manipulated byte back to the process.
  if (!errno) {
    data ^= (1 << bit);
    err = ptrace(PTRACE_POKEDATA, pid, addr, data);
    n_write_errs += !!err;
    found_errs |= !!err;
  }
  printf(" (%s)\n", found_errs ? "Failed" : "OK");
}

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
    F(mm, "Error allocating memory for a map entry.");

    // Read in the start and end memory range value, and the permission field.
    char *range = strtok(line, " ");
    F(range, "Obtaining range value.");
    const char *perm = strtok(NULL, " ");
    F(perm, "Obtaining the permission field.");
    memcpy(mm->permissions, perm, sizeof(mm->permissions));

    // Convert the range to integer values.
    const char *sep = strchr(range, '-');
    F(sep, "Obtaining range separator character.");
    range[sep - range] = '\0';
    mm->start = strtoll(range, NULL, 16);
    mm->end = strtoll(sep + 1, NULL, 16);
    F(mm->start, "Obtaining the range start value.");
    F(mm->end, "Obtaining the range end value.");

    // Skip offset,dev,inode and get the pathname field (last field).
    strtok(NULL, " ");  // Offset
    strtok(NULL, " ");  // Device
    strtok(NULL, " ");  // Inode
    char *path = strtok(NULL, " ");
    if (path && strlen(path) > 1) {
      mm->pathname = strdup(path);
      mm->pathname[strlen(mm->pathname) - 1] = '\0';
    }

#if __UINTPTR_MAX__ == __UINT64_MAX__
    if (mm->start & ~0x00007fffffffffff) {
      // Ignore this region if it belongs to kernel space.
      // https://www.kernel.org/doc/Documentation/x86/x86_64/mm.txt
      free(mm);
      continue;
    }
#else
#error "Unxepected word size."
#endif

    mm->next = head;
    head = mm;
    ++*count;
  }

  fclose(fp);
  return head;
}

int main(int argc, char **argv) {
  srand(time(NULL));
  int n_iterations = 1, opt;
  pid_t pid = 0;
  while ((opt = getopt(argc, argv, "p:n:h")) != -1) {
    switch (opt) {
      case 'p':
        pid = atoi(optarg);
        break;
      case 'n':
        n_iterations = atoi(optarg);
        break;
      case 'h':
      default:
        usage(argv[0]);
    }
  }
  if (pid == 0) F(0, "Invalid arguments, -p must be specified.");
  if (n_iterations < 1) F(0, "Invalid number of iterations (see -h).");

  // Attach to pid.
  long err = ptrace(PTRACE_SEIZE, pid, 0, 0);
  FE(err == -1, "Error seizing target process %d", pid);

  unsigned n_regions = 0;
  memory_regions_t regions = get_memory_map(pid, &n_regions);
  L("Found %u candidate memory regions to write to.", n_regions);
  for (int i = 0; i < n_iterations; ++i)
    attack(pid, regions, n_regions, i, n_iterations);

  printf("[+] Write Errors: %d of %d\n", n_write_errs, n_iterations);
  printf("[+] Read Errors:  %d of %d\n", n_read_errs, n_iterations);
  return 0;
}
