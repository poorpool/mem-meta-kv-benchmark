// NOLINTBEGIN
#pragma once
#include "common.h"
#include <dirent.h>
#include <numaif.h>
#include <numa.h>
class HugeCtx {

  struct HugepageInfo {
    uint64_t hugepage_sz; /**< size of a huge page */
    string hugedir;       /**< dir where hugetlbfs is mounted */
    const bool operator<(const HugepageInfo &hpi) {
      return hugepage_sz > hpi.hugepage_sz;
    }
  };

  bool inited;
  string name;
  atomic<int32_t> seg_id;
  vector<HugepageInfo> infos;

public:
  HugeCtx() : inited(false), seg_id(0) {}
  bool is_inited() { return inited; }
  void init(string name) {
    p_assert(inited == false);
    this->name = name;
    string dirent_start_text = "hugepages-";
    DIR *dir;
    struct dirent *dirent;

    dir = opendir("/sys/kernel/mm/hugepages");
    p_assert(dir);

    for (dirent = readdir(dir); dirent != NULL; dirent = readdir(dir)) {
      struct HugepageInfo hpi;

      if (strncmp(dirent->d_name, dirent_start_text.c_str(),
                  dirent_start_text.size()) != 0)
        continue;

      hpi.hugepage_sz =
          mcat_str_to_size(&dirent->d_name[dirent_start_text.size()]);

      /* first, check if we have a mountpoint */
      if (get_hugepage_dir(hpi.hugepage_sz, hpi.hugedir) < 0) {
        continue;
      }
      infos.push_back(hpi);
    }
    closedir(dir);

    p_assert(dirent == NULL);

    /* sort the page directory entries by size, largest to smallest */
    std::sort(infos.begin(), infos.end());
    inited = true;
  }

  void *allocSeg(size_t size, int32_t numa_id) {
    if (inited == false || infos.empty())
      return NULL;
    int32_t id = seg_id++;
    // chose the largest page
    HugepageInfo &hpi = infos[0];
    size = ((size + hpi.hugepage_sz - 1) / hpi.hugepage_sz) * hpi.hugepage_sz;
    string path = hpi.hugedir + "/" + name + "-" + to_string(id);
    p_info("Huge Seg file create: %s", path.c_str());
    unlink(path.c_str());
    int fd = open(path.c_str(), O_CREAT | O_RDWR, 0600);
    p_assert(fd >= 0);
    p_assert(0 == ftruncate(fd, size));
    void *va = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    p_err_assert(va != MAP_FAILED, "Huge space not enough");

    // numa aware
    p_assert(numa_available() != -1);
    numa_set_localalloc();

    int32_t cur_numa_id;
    p_assert(0 == get_mempolicy(&cur_numa_id, NULL, 0, va,
                                MPOL_F_NODE | MPOL_F_ADDR));
    if (cur_numa_id != numa_id) {
      p_info("Huge seg current_numa=%d req_numa=%d", cur_numa_id, numa_id);

      munmap(va, size);
      struct bitmask *old_mask = numa_allocate_nodemask();
      struct bitmask *new_mask = numa_allocate_nodemask();
      int32_t old_policy;
      p_assert(0 == get_mempolicy(&old_policy, old_mask->maskp,
                                  old_mask->size + 1, 0, 0));

      numa_bitmask_setbit(numa_bitmask_clearall(new_mask), numa_id);
      set_mempolicy(MPOL_BIND, new_mask->maskp, new_mask->size + 1);

      p_assert(0 == ftruncate(fd, 0) && 0 == ftruncate(fd, size));
      va = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      memset(va, 0, size);

      // restore old mask
      if (MPOL_DEFAULT == old_policy) {
        numa_set_localalloc();
      } else {
        p_assert(0 == set_mempolicy(old_policy, old_mask->maskp,
                                    old_mask->size + 1));
      }
      numa_free_nodemask(old_mask);
      numa_free_nodemask(new_mask);
    } else {
      p_info("Huge seg current_numa=req_numa=%d", numa_id);
    }
    int h2s = vma2numa(va);
    p_err_assert(h2s == numa_id, "h2s wrong vma %d expect %d", h2s, numa_id);
    return va;
  }
  // Seg Release is not supported for now

private:
  static int strsplit(char *string, int stringlen, char **tokens, int maxtokens,
                      char delim) {
    int i, tok = 0;
    int tokstart = 1; /* first token is right at start of string */

    if (string == NULL || tokens == NULL)
      goto einval_error;

    for (i = 0; i < stringlen; i++) {
      if (string[i] == '\0' || tok >= maxtokens)
        break;
      if (tokstart) {
        tokstart = 0;
        tokens[tok++] = &string[i];
      }
      if (string[i] == delim) {
        string[i] = '\0';
        tokstart = 1;
      }
    }
    return tok;

  einval_error:
    errno = EINVAL;
    return -1;
  }

  static uint64_t get_default_hp_size(void) {
    const char proc_meminfo[] = "/proc/meminfo";
    const char str_hugepagesz[] = "Hugepagesize:";
    unsigned hugepagesz_len = sizeof(str_hugepagesz) - 1;
    char buffer[256];
    unsigned long long size = 0;

    FILE *fd = fopen(proc_meminfo, "r");
    p_assert(fd);
    while (fgets(buffer, sizeof(buffer), fd)) {
      if (strncmp(buffer, str_hugepagesz, hugepagesz_len) == 0) {
        size = mcat_str_to_size(&buffer[hugepagesz_len]);
        break;
      }
    }
    fclose(fd);
    p_assert(size);
    return size;
  }

  static int get_hugepage_dir(uint64_t hugepage_sz, string &hugedir) {
    enum proc_mount_fieldnames {
      DEVICE = 0,
      MOUNTPT,
      FSTYPE,
      OPTIONS,
      _FIELDNAME_MAX
    };
    const char proc_mounts[] = "/proc/mounts";
    const char hugetlbfs_str[] = "hugetlbfs";
    const size_t htlbfs_str_len = sizeof(hugetlbfs_str) - 1;
    const char pagesize_opt[] = "pagesize=";
    const size_t pagesize_opt_len = sizeof(pagesize_opt) - 1;
    const char split_tok = ' ';
    char *splitstr[_FIELDNAME_MAX];
    char found[PATH_MAX] = "";
    char buf[BUFSIZ];
    uint64_t default_size = get_default_hp_size();
    FILE *fd = fopen(proc_mounts, "r");
    p_assert(fd);

    while (fgets(buf, sizeof(buf), fd)) {
      const char *pagesz_str;

      if (strsplit(buf, sizeof(buf), splitstr, _FIELDNAME_MAX, split_tok) !=
          _FIELDNAME_MAX) {
        p_assert(0);
        // break;
      }

      if (strncmp(splitstr[FSTYPE], hugetlbfs_str, htlbfs_str_len) != 0)
        continue;

      pagesz_str = strstr(splitstr[OPTIONS], pagesize_opt);

      /* if no explicit page size, the default page size is compared */
      if (pagesz_str == NULL) {
        if (hugepage_sz != default_size)
          continue;
      }
      /* there is an explicit page size, so check it */
      else {
        uint64_t pagesz = mcat_str_to_size(&pagesz_str[pagesize_opt_len]);
        if (pagesz != hugepage_sz)
          continue;
      }

      strcpy(found, splitstr[MOUNTPT]);
      break;
    } /* end while fgets */

    fclose(fd);

    if (found[0] != '\0') {
      hugedir = string(found);
      return 0;
    }
    hugedir = "";
    return -1;
  }
};
// NOLINTEND