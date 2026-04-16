#include "base/io.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>

namespace pfd::base {

namespace {

QString makeIoDetail(const QString& path, QFileDevice::FileError fe) {
  return QStringLiteral("path=%1;qt_file_error=%2")
      .arg(path, QString::number(static_cast<int>(fe)));
}

ErrorCode mapFileErrorToCode(QFileDevice::FileError fe, ErrorCode defaultCode) {
  switch (fe) {
    case QFileDevice::NoError:
      return ErrorCode::kSuccess;
    case QFileDevice::ReadError:
      return ErrorCode::kFileReadFailed;
    case QFileDevice::WriteError:
      return ErrorCode::kFileWriteFailed;
    case QFileDevice::ResourceError:
      return ErrorCode::kDiskFull;
    case QFileDevice::OpenError:
      return defaultCode;
    case QFileDevice::PermissionsError:
      return ErrorCode::kPathPermissionDenied;
    default:
      return defaultCode;
  }
}

Error errorForFile(QFileDevice& f, ErrorCode defaultCode, const QString& context) {
  const QFileDevice::FileError fe = f.error();
  if (fe == QFileDevice::NoError) {
    return Error();
  }

  const ErrorCode code = mapFileErrorToCode(fe, defaultCode);
  const QString msg =
      context.isEmpty() ? f.errorString() : QStringLiteral("%1: %2").arg(context, f.errorString());
  return Error(code, msg, makeIoDetail(f.fileName(), fe), ErrorDomain::kIo, static_cast<int>(fe));
}

}  // namespace

std::pair<QByteArray, Error> readWholeFile(const QString& path) {
  if (!QFileInfo::exists(path)) {
    return {QByteArray(), Error(ErrorCode::kFileNotFound, QStringLiteral("File does not exist"),
                                makeIoDetail(path, QFileDevice::OpenError), ErrorDomain::kIo,
                                static_cast<int>(QFileDevice::OpenError))};
  }

  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) {
    return {QByteArray(),
            errorForFile(f, ErrorCode::kFileReadFailed, QStringLiteral("Cannot open for read"))};
  }
  const QByteArray data = f.readAll();
  if (f.error() != QFileDevice::NoError) {
    return {QByteArray(),
            errorForFile(f, ErrorCode::kFileReadFailed, QStringLiteral("Read failed"))};
  }
  return {data, Error()};
}

std::pair<QString, Error> readWholeFileAsText(const QString& path) {
  auto [data, err] = readWholeFile(path);
  if (err.hasError()) {
    return {QString(), err};
  }
  return {QString::fromUtf8(data), Error()};
}

Error writeWholeFile(const QString& path, const QByteArray& data) {
  QSaveFile f(path);
  if (!f.open(QIODevice::WriteOnly)) {
    return errorForFile(f, ErrorCode::kFileWriteFailed, QStringLiteral("Cannot open for write"));
  }
  if (f.write(data) != data.size()) {
    return errorForFile(f, ErrorCode::kFileWriteFailed, QStringLiteral("Write failed"));
  }
  if (!f.commit()) {
    const QFileDevice::FileError fe = f.error();
    return Error(ErrorCode::kFileWriteFailed, QStringLiteral("Atomic commit (rename) failed"),
                 makeIoDetail(path, fe), ErrorDomain::kIo, static_cast<int>(fe));
  }
  return Error();
}

Error writeWholeFile(const QString& path, const QString& text) {
  return writeWholeFile(path, text.toUtf8());
}

Error writeWholeFileWithBackup(const QString& path, const QByteArray& data) {
  if (QFileInfo::exists(path)) {
    const QString bak = path + QStringLiteral(".bak");
    const QString bak2 = path + QStringLiteral(".bak2");
    if (QFileInfo::exists(bak)) {
      QFile::remove(bak2);
      QFile::copy(bak, bak2);
    }
    QFile::remove(bak);
    QFile::copy(path, bak);
  }
  return writeWholeFile(path, data);
}

RecoveryReadResult readWholeFileWithRecovery(const QString& path) {
  const QStringList candidates = {
      path,
      path + QStringLiteral(".bak"),
      path + QStringLiteral(".bak2"),
  };
  for (const auto& candidate : candidates) {
    auto [data, err] = readWholeFile(candidate);
    if (!err.hasError() && !data.isEmpty()) {
      return {data, candidate, Error()};
    }
  }
  return {QByteArray(), QString(),
          Error(ErrorCode::kFileNotFound,
                QStringLiteral("No readable copy found (primary + backups)"),
                QStringLiteral("path=%1").arg(path), ErrorDomain::kIo, 0)};
}

QString sha256Hex(const QByteArray& data) {
  return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

}  // namespace pfd::base
