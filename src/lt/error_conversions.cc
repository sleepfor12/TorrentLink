#include "lt/error_conversions.h"

#include <cerrno>

namespace pfd::lt {
namespace {

QString makeDetail(const QString& context, int rawCode) {
  if (context.isEmpty()) {
    return QStringLiteral("lt_raw_code=%1").arg(rawCode);
  }
  return QStringLiteral("%1 | lt_raw_code=%2").arg(context).arg(rawCode);
}

}  // namespace

pfd::base::Error fromLibtorrentError(int rawCode, const QString& rawMessage, const QString& context,
                                     LtErrorHint hint) {
  using pfd::base::Error;
  using pfd::base::ErrorCode;
  using pfd::base::ErrorDomain;

  if (rawCode == 0 && rawMessage.trimmed().isEmpty()) {
    return Error();
  }

  const QString detail = makeDetail(context, rawCode);
  switch (hint) {
    case LtErrorHint::kAddTorrent:
      return Error(ErrorCode::kAddTorrentFailed, rawMessage, detail, ErrorDomain::kLibtorrent,
                   rawCode);
    case LtErrorHint::kMetadata:
      return Error(ErrorCode::kMetadataTimeout, rawMessage, detail, ErrorDomain::kLibtorrent,
                   rawCode);
    case LtErrorHint::kStorage:
      if (rawCode == ENOSPC) {
        return Error(ErrorCode::kDiskFull, rawMessage, detail, ErrorDomain::kLibtorrent, rawCode);
      }
      return Error(ErrorCode::kFileWriteFailed, rawMessage, detail, ErrorDomain::kLibtorrent,
                   rawCode);
    case LtErrorHint::kNetwork:
      if (rawCode == ETIMEDOUT) {
        return Error(ErrorCode::kMetadataTimeout, rawMessage, detail, ErrorDomain::kLibtorrent,
                     rawCode);
      }
      return Error(ErrorCode::kUnknown, rawMessage, detail, ErrorDomain::kLibtorrent, rawCode);
    case LtErrorHint::kGeneric:
      return Error(ErrorCode::kUnknown, rawMessage, detail, ErrorDomain::kLibtorrent, rawCode);
  }

  return Error(ErrorCode::kUnknown, rawMessage, detail, ErrorDomain::kLibtorrent, rawCode);
}

pfd::base::Error fromLibtorrentError(const std::error_code& ec, const QString& context,
                                     LtErrorHint hint) {
  if (!ec) {
    return pfd::base::Error();
  }
  return fromLibtorrentError(ec.value(), QString::fromStdString(ec.message()), context, hint);
}

}  // namespace pfd::lt
