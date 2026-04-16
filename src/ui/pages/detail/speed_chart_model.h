#ifndef PFD_UI_PAGES_DETAIL_SPEED_CHART_MODEL_H
#define PFD_UI_PAGES_DETAIL_SPEED_CHART_MODEL_H

#include <QtCore/QObject>
#include <QtCore/QVariantList>

#include <array>

namespace pfd::ui {

class SpeedChartModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList dlSamples READ dlSamples NOTIFY samplesChanged)
  Q_PROPERTY(QVariantList ulSamples READ ulSamples NOTIFY samplesChanged)
  Q_PROPERTY(double maxRate READ maxRate NOTIFY samplesChanged)
  Q_PROPERTY(QString dlCurrent READ dlCurrent NOTIFY samplesChanged)
  Q_PROPERTY(QString ulCurrent READ ulCurrent NOTIFY samplesChanged)
  Q_PROPERTY(double dlCurrentValue READ dlCurrentValue NOTIFY samplesChanged)
  Q_PROPERTY(double ulCurrentValue READ ulCurrentValue NOTIFY samplesChanged)
  Q_PROPERTY(int sampleCount READ sampleCount NOTIFY samplesChanged)

public:
  static constexpr int kMaxSamples = 4096;

  explicit SpeedChartModel(QObject* parent = nullptr);

  void addSample(qint64 downloadRate, qint64 uploadRate, qint64 payloadDownloadRate = -1,
                 qint64 payloadUploadRate = -1, qint64 overheadDownloadRate = 0,
                 qint64 overheadUploadRate = 0, qint64 dhtDownloadRate = 0,
                 qint64 dhtUploadRate = 0, qint64 trackerDownloadRate = 0,
                 qint64 trackerUploadRate = 0);
  void clear();

  [[nodiscard]] QVariantList dlSamples() const;
  [[nodiscard]] QVariantList ulSamples() const;
  [[nodiscard]] double maxRate() const;
  [[nodiscard]] QString dlCurrent() const;
  [[nodiscard]] QString ulCurrent() const;
  [[nodiscard]] double dlCurrentValue() const;
  [[nodiscard]] double ulCurrentValue() const;
  [[nodiscard]] int sampleCount() const;

  Q_INVOKABLE QString formatRate(double bytesPerSec) const;
  Q_INVOKABLE QVariantList seriesSamples(const QString& key, int windowMinutes) const;
  Q_INVOKABLE double maxRateForSeries(const QStringList& keys, int windowMinutes) const;

Q_SIGNALS:
  void samplesChanged();

private:
  struct Sample {
    qint64 ts_ms{0};
    qint64 dl{0};
    qint64 ul{0};
    qint64 payload_dl{0};
    qint64 payload_ul{0};
    qint64 overhead_dl{0};
    qint64 overhead_ul{0};
    qint64 dht_dl{0};
    qint64 dht_ul{0};
    qint64 tracker_dl{0};
    qint64 tracker_ul{0};
  };

  std::array<Sample, kMaxSamples> samples_{};
  int head_{0};
  int count_{0};
};

}  // namespace pfd::ui

#endif  // PFD_UI_PAGES_DETAIL_SPEED_CHART_MODEL_H
