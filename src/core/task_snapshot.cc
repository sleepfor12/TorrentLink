#include "core/task_snapshot.h"

namespace pfd::core {

double TaskSnapshot::progress01() const {
  if (totalBytes <= 0) {
    return 0.0;
  }
  const double p = static_cast<double>(downloadedBytes) / static_cast<double>(totalBytes);
  if (p < 0.0) {
    return 0.0;
  }
  if (p > 1.0) {
    return 1.0;
  }
  return p;
}

}  // namespace pfd::core
