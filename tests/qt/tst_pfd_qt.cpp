#include <QtCore/QUuid>
#include <QtTest/QtTest>

#include "base/format.h"
#include "core/task_event.h"
#include "lt/task_event_mapper.h"

class TstPfdQt : public QObject {
  Q_OBJECT

 private slots:
  void formatBytesIec_smoke();
  void mapTaskErrorAlert_toEvent_carriesMessage();
  void mapDuplicateRejected_toTaskEvent();
};

void TstPfdQt::formatBytesIec_smoke() {
  const QString s = pfd::base::formatBytesIec(1024);
  QVERIFY(!s.isEmpty());
  QVERIFY(s.contains(QLatin1Char('i')) || s.contains(QLatin1Char('K')) ||
          s.contains(QLatin1Char('k')));
}

void TstPfdQt::mapTaskErrorAlert_toEvent_carriesMessage() {
  pfd::lt::LtAlertView v;
  v.type = pfd::lt::LtAlertView::Type::kTaskError;
  v.taskId = QUuid::createUuid();
  v.message = QStringLiteral("generic failure");
  const auto ev = pfd::lt::mapAlertToTaskEvent(v);
  QVERIFY(ev.has_value());
  QCOMPARE(ev->type, pfd::core::TaskEventType::kErrorOccurred);
  QCOMPARE(ev->errorMessage, v.message);
}

void TstPfdQt::mapDuplicateRejected_toTaskEvent() {
  pfd::lt::LtAlertView v;
  v.type = pfd::lt::LtAlertView::Type::kDuplicateRejected;
  v.taskId = QUuid::createUuid();
  v.message = QStringLiteral("任务已存在（相同 info-hash），已忽略重复添加");
  const auto ev = pfd::lt::mapAlertToTaskEvent(v);
  QVERIFY(ev.has_value());
  QCOMPARE(ev->type, pfd::core::TaskEventType::kDuplicateRejected);
  QCOMPARE(ev->errorMessage, v.message);
}

QTEST_MAIN(TstPfdQt)
#include "tst_pfd_qt.moc"
