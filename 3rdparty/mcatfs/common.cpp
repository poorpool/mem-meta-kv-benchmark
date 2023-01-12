// NOLINTBEGIN

#include "common.h"

char *mcat_get_hostname() {
  static char hn[MCAT_PATH_MAX_LEN] = {0};
  static mutex l;
  if (!hn[0]) {
    l.lock();
    if (!hn[0]) {
      gethostname(hn, MCAT_PATH_MAX_LEN);
      if (!hn[0])
        abort();
    }
    l.unlock();
  }
  return hn;
}
// NOLINTEND