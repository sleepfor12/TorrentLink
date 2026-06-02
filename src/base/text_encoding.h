#ifndef PFD_BASE_TEXT_ENCODING_H
#define PFD_BASE_TEXT_ENCODING_H

#include <QtCore/QString>

#include <string>

namespace pfd::base {

/// Returns true when @p data is well-formed UTF-8 (no overlong or truncated sequences).
[[nodiscard]] bool isValidUtf8(const char* data, int size);

/// Decodes bytes from libtorrent / system APIs: prefer UTF-8, fall back to locale on Windows.
[[nodiscard]] QString QStringFromStdBytes(const std::string& bytes);

/// Replaces non-printable control characters for safe UI display.
[[nodiscard]] QString sanitizeHumanReadableText(const QString& text);

}  // namespace pfd::base

#endif  // PFD_BASE_TEXT_ENCODING_H
