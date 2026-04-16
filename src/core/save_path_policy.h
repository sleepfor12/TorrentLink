#ifndef PFD_CORE_SAVE_PATH_POLICY_H
#define PFD_CORE_SAVE_PATH_POLICY_H

#include <QtCore/QString>

namespace pfd::core {

enum class ContentLayout {
  kOriginal = 0,
  kCreateSubfolder,
  kNoSubfolder,
};

class SavePathPolicy {
public:
  void setDefaultDownloadDir(const QString& dir);

  [[nodiscard]] QString resolve(const QString& input) const;

  [[nodiscard]] QString applyLayout(const QString& basePath,
                                    const QString& torrentName,
                                    ContentLayout layout) const;

  static void ensureExists(const QString& path);

private:
  QString default_dir_;
};

}  // namespace pfd::core

#endif  // PFD_CORE_SAVE_PATH_POLICY_H
