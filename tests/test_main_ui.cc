#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtWidgets/QApplication>

#include <gtest/gtest.h>

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
  const QString runtimeDir = QDir::temp().filePath(QStringLiteral("torrentlink-test-runtime"));
  QDir().mkpath(runtimeDir);
  QFile::setPermissions(runtimeDir,
                        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
  qputenv("XDG_RUNTIME_DIR", runtimeDir.toLocal8Bit());
  QApplication app(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
