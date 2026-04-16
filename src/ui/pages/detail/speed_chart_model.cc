#include "ui/pages/detail/speed_chart_model.h"

#include <algorithm>
#include <limits>

#include "base/format.h"
#include "base/time_stamps.h"

namespace pfd::ui {

SpeedChartModel::SpeedChartModel(QObject* parent) : QObject(parent) {}

void SpeedChartModel::addSample(qint64 downloadRate, qint64 uploadRate, qint64 payloadDownloadRate,
                                qint64 payloadUploadRate, qint64 overheadDownloadRate,
                                qint64 overheadUploadRate, qint64 dhtDownloadRate,
                                qint64 dhtUploadRate, qint64 trackerDownloadRate,
                                qint64 trackerUploadRate) {
  const int idx = (head_ + count_) % kMaxSamples;
  samples_[static_cast<std::size_t>(idx)] = {
      pfd::base::currentMs(),
      downloadRate,
      uploadRate,
      payloadDownloadRate >= 0 ? payloadDownloadRate : downloadRate,
      payloadUploadRate >= 0 ? payloadUploadRate : uploadRate,
      overheadDownloadRate,
      overheadUploadRate,
      dhtDownloadRate,
      dhtUploadRate,
      trackerDownloadRate,
      trackerUploadRate,
  };
  if (count_ < kMaxSamples) {
    ++count_;
  } else {
    head_ = (head_ + 1) % kMaxSamples;
  }
  Q_EMIT samplesChanged();
}

void SpeedChartModel::clear() {
  samples_ = {};
  head_ = 0;
  count_ = 0;
  Q_EMIT samplesChanged();
}

QVariantList SpeedChartModel::dlSamples() const {
  QVariantList out;
  out.reserve(count_);
  for (int i = 0; i < count_; ++i) {
    out.append(static_cast<double>(samples_[static_cast<std::size_t>((head_ + i) % kMaxSamples)].dl));
  }
  return out;
}

QVariantList SpeedChartModel::ulSamples() const {
  QVariantList out;
  out.reserve(count_);
  for (int i = 0; i < count_; ++i) {
    out.append(static_cast<double>(samples_[static_cast<std::size_t>((head_ + i) % kMaxSamples)].ul));
  }
  return out;
}

double SpeedChartModel::maxRate() const {
  qint64 mx = 1024;
  for (int i = 0; i < count_; ++i) {
    const auto& s = samples_[static_cast<std::size_t>((head_ + i) % kMaxSamples)];
    mx = std::max({mx, s.dl, s.ul});
  }
  return static_cast<double>(mx);
}

QString SpeedChartModel::dlCurrent() const {
  if (count_ == 0) return QStringLiteral("0 B/s");
  return pfd::base::formatRate(samples_[static_cast<std::size_t>((head_ + count_ - 1) % kMaxSamples)].dl);
}

QString SpeedChartModel::ulCurrent() const {
  if (count_ == 0) return QStringLiteral("0 B/s");
  return pfd::base::formatRate(samples_[static_cast<std::size_t>((head_ + count_ - 1) % kMaxSamples)].ul);
}

double SpeedChartModel::dlCurrentValue() const {
  if (count_ == 0) return 0.0;
  return static_cast<double>(
      samples_[static_cast<std::size_t>((head_ + count_ - 1) % kMaxSamples)].dl);
}

double SpeedChartModel::ulCurrentValue() const {
  if (count_ == 0) return 0.0;
  return static_cast<double>(
      samples_[static_cast<std::size_t>((head_ + count_ - 1) % kMaxSamples)].ul);
}

int SpeedChartModel::sampleCount() const { return count_; }

QString SpeedChartModel::formatRate(double bytesPerSec) const {
  return pfd::base::formatRate(static_cast<qint64>(bytesPerSec));
}

QVariantList SpeedChartModel::seriesSamples(const QString& key, int windowMinutes) const {
  const auto seriesValue = [&](const Sample& s) -> qint64 {
    if (key == QStringLiteral("payload_dl")) return s.payload_dl;
    if (key == QStringLiteral("payload_ul")) return s.payload_ul;
    if (key == QStringLiteral("overhead_dl")) return s.overhead_dl;
    if (key == QStringLiteral("overhead_ul")) return s.overhead_ul;
    if (key == QStringLiteral("dht_dl")) return s.dht_dl;
    if (key == QStringLiteral("dht_ul")) return s.dht_ul;
    if (key == QStringLiteral("tracker_dl")) return s.tracker_dl;
    if (key == QStringLiteral("tracker_ul")) return s.tracker_ul;
    if (key == QStringLiteral("dl")) return s.dl;
    if (key == QStringLiteral("ul")) return s.ul;
    return 0;
  };
  QVariantList out;
  if (count_ <= 0) return out;

  const qint64 nowMs =
      samples_[static_cast<std::size_t>((head_ + count_ - 1) % kMaxSamples)].ts_ms;
  const qint64 fromMs = windowMinutes > 0 ? (nowMs - static_cast<qint64>(windowMinutes) * 60 * 1000)
                                          : std::numeric_limits<qint64>::min();
  out.reserve(count_);
  for (int i = 0; i < count_; ++i) {
    const auto& s = samples_[static_cast<std::size_t>((head_ + i) % kMaxSamples)];
    if (s.ts_ms < fromMs) continue;
    out.append(static_cast<double>(seriesValue(s)));
  }
  if (out.isEmpty()) {
    out.append(0.0);
    out.append(0.0);
  } else if (out.size() == 1) {
    out.append(out.front());
  }
  return out;
}

double SpeedChartModel::maxRateForSeries(const QStringList& keys, int windowMinutes) const {
  const auto seriesValue = [](const Sample& s, const QString& key) -> qint64 {
    if (key == QStringLiteral("payload_dl")) return s.payload_dl;
    if (key == QStringLiteral("payload_ul")) return s.payload_ul;
    if (key == QStringLiteral("overhead_dl")) return s.overhead_dl;
    if (key == QStringLiteral("overhead_ul")) return s.overhead_ul;
    if (key == QStringLiteral("dht_dl")) return s.dht_dl;
    if (key == QStringLiteral("dht_ul")) return s.dht_ul;
    if (key == QStringLiteral("tracker_dl")) return s.tracker_dl;
    if (key == QStringLiteral("tracker_ul")) return s.tracker_ul;
    if (key == QStringLiteral("dl")) return s.dl;
    if (key == QStringLiteral("ul")) return s.ul;
    return 0;
  };

  if (count_ <= 0 || keys.isEmpty()) return 1024.0;
  const qint64 nowMs =
      samples_[static_cast<std::size_t>((head_ + count_ - 1) % kMaxSamples)].ts_ms;
  const qint64 fromMs = windowMinutes > 0 ? (nowMs - static_cast<qint64>(windowMinutes) * 60 * 1000)
                                          : std::numeric_limits<qint64>::min();
  qint64 mx = 1024;
  for (int i = 0; i < count_; ++i) {
    const auto& s = samples_[static_cast<std::size_t>((head_ + i) % kMaxSamples)];
    if (s.ts_ms < fromMs) continue;
    for (const auto& k : keys) mx = std::max(mx, seriesValue(s, k));
  }
  return static_cast<double>(mx);
}

}  // namespace pfd::ui
