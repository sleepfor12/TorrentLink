#include "ui/search_tab.h"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>

#include "base/format.h"

namespace pfd::ui {

SearchTab::SearchTab(QWidget* parent) : QWidget(parent) {
  buildLayout();
}

void SearchTab::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 16);
  root->setSpacing(12);

  auto* title = new QLabel(QStringLiteral("搜索"), this);
  title->setStyleSheet(QStringLiteral("font-size:18px;font-weight:700;color:#1f2d3d;"));
  root->addWidget(title);

  auto* row = new QHBoxLayout();
  row->setSpacing(10);
  queryEdit_ = new QLineEdit(this);
  queryEdit_->setPlaceholderText(QStringLiteral("输入关键字（例如：Ubuntu ISO）"));
  queryEdit_->setClearButtonEnabled(true);
  scopeBox_ = new QComboBox(this);
  scopeBox_->addItems(
      QStringList{QStringLiteral("本地历史"), QStringLiteral("RSS"), QStringLiteral("全站")});
  searchBtn_ = new QPushButton(QStringLiteral("搜索"), this);
  searchBtn_->setObjectName(QStringLiteral("PrimaryButton"));
  row->addWidget(queryEdit_, 1);
  row->addWidget(scopeBox_);
  row->addWidget(searchBtn_);
  root->addLayout(row);

  auto* splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);
  resultList_ = new QListWidget(splitter);
  resultView_ = new QTextBrowser(splitter);
  resultView_->setOpenExternalLinks(true);
  resultView_->setPlaceholderText(QStringLiteral("选择一条搜索结果后在此查看详情。"));
  splitter->addWidget(resultList_);
  splitter->addWidget(resultView_);
  splitter->setStretchFactor(0, 2);
  splitter->setStretchFactor(1, 5);

  root->addWidget(splitter, 1);

  connect(searchBtn_, &QPushButton::clicked, this, &SearchTab::performSearch);
  connect(queryEdit_, &QLineEdit::returnPressed, this, &SearchTab::performSearch);
  connect(resultList_, &QListWidget::currentRowChanged, this, &SearchTab::onResultSelected);
}

void SearchTab::setQuerySnapshotsFn(QuerySnapshotsFn fn) {
  querySnapshotsFn_ = std::move(fn);
}
void SearchTab::setQueryRssItemsFn(QueryRssItemsFn fn) {
  queryRssItemsFn_ = std::move(fn);
}

void SearchTab::setRequestHeaders(RequestHeaders headers) {
  requestHeaders_ = std::move(headers);
}

void SearchTab::performSearch() {
  const QString keyword = queryEdit_->text().trimmed();
  if (keyword.isEmpty())
    return;

  results_.clear();
  resultList_->clear();
  resultView_->clear();

  const int scope = scopeBox_->currentIndex();

  if (scope == 0) {
    if (!querySnapshotsFn_)
      return;
    const auto snapshots = querySnapshotsFn_();
    for (const auto& s : snapshots) {
      if (s.name.contains(keyword, Qt::CaseInsensitive)) {
        ResultEntry entry;
        entry.title = s.name;
        entry.detail =
            QStringLiteral("<b>%1</b><br>"
                           "状态: %2<br>"
                           "大小: %3<br>"
                           "路径: %4<br>"
                           "InfoHash v1: %5<br>"
                           "InfoHash v2: %6")
                .arg(s.name.toHtmlEscaped(),
                     s.status == pfd::base::TaskStatus::kFinished      ? QStringLiteral("已完成")
                     : s.status == pfd::base::TaskStatus::kDownloading ? QStringLiteral("下载中")
                     : s.status == pfd::base::TaskStatus::kPaused      ? QStringLiteral("已暂停")
                                                                       : QStringLiteral("其他"),
                     pfd::base::formatBytes(s.totalBytes), s.savePath.toHtmlEscaped(),
                     s.infoHashV1.isEmpty() ? QStringLiteral("--") : s.infoHashV1,
                     s.infoHashV2.isEmpty() ? QStringLiteral("--") : s.infoHashV2);
        results_.push_back(std::move(entry));
      }
    }
  } else if (scope == 1) {
    if (!queryRssItemsFn_)
      return;
    const auto items = queryRssItemsFn_(keyword);
    for (const auto& [title, link] : items) {
      ResultEntry entry;
      entry.title = title;
      entry.detail =
          QStringLiteral("<b>%1</b><br>链接: %2").arg(title.toHtmlEscaped(), link.toHtmlEscaped());
      results_.push_back(std::move(entry));
    }
  } else {
    QMessageBox::information(
        this, QStringLiteral("提示"),
        QStringLiteral(
            "在线搜索暂未支持，请使用「本地历史」或「RSS」。\n当前请求头配置将用于后续在线搜索：\n"
            "User-Agent: %1\nAccept-Language: %2")
            .arg(requestHeaders_.user_agent, requestHeaders_.accept_language));
    return;
  }

  for (const auto& r : results_) {
    resultList_->addItem(r.title);
  }

  if (results_.empty()) {
    resultView_->setHtml(QStringLiteral("<p style='color:#9ca3af'>未找到匹配结果。</p>"));
  }
}

void SearchTab::onResultSelected(int row) {
  if (row < 0 || row >= static_cast<int>(results_.size())) {
    resultView_->clear();
    return;
  }
  resultView_->setHtml(results_[static_cast<size_t>(row)].detail);
}

}  // namespace pfd::ui
