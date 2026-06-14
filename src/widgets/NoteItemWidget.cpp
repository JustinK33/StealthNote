#include "NoteItemWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QIcon>
#include <QSizePolicy>

#include <QDateTime>

namespace {
QString firstNonEmptyLine(const QString &text) {
  const auto lines = text.split('\n');
  for (const auto &line : lines) {
    const QString trimmed = line.trimmed();
    if (!trimmed.isEmpty()) {
      return trimmed;
    }
  }
  return "Untitled";
}

QString relativeTime(const QDateTime &dateTime) {
  if (!dateTime.isValid()) {
    return "just now";
  }

  const QDateTime now = QDateTime::currentDateTime();
  qint64 seconds = dateTime.secsTo(now);
  if (seconds < 60) {
    return "just now";
  }
  if (seconds < 3600) {
    return QString("%1m ago").arg(seconds / 60);
  }
  if (seconds < 86400) {
    return QString("%1h ago").arg(seconds / 3600);
  }
  if (seconds < 172800) {
    return "Yesterday";
  }
  return dateTime.toString("MMM d");
}
} // namespace

NoteItemWidget::NoteItemWidget(const Note &note, QWidget *parent)
    : QWidget(parent), id(note.id) {
  setObjectName("NoteItem");
  setProperty("pinned", note.pinned);

  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(10, 6, 10, 6);
  layout->setSpacing(10);

  auto *textLayout = new QVBoxLayout();
  textLayout->setSpacing(0);

  const QString title = firstNonEmptyLine(note.body);
  const QString time = relativeTime(note.updated);
  titleLabel = new QLabel(QString("%1 (%2)").arg(title, time), this);
  titleLabel->setObjectName("NoteTitle");
  titleLabel->setWordWrap(false);
  titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  textLayout->addWidget(titleLabel);

  auto *actionsLayout = new QHBoxLayout();
  actionsLayout->setSpacing(8);

  auto *pinButton = new QToolButton(this);
  pinButton->setObjectName("IconButton");
  pinButton->setIcon(QIcon(":/icons/pin.svg"));
  pinButton->setIconSize(QSize(12, 12));
  pinButton->setAutoRaise(true);

  auto *editButton = new QToolButton(this);
  editButton->setObjectName("IconButton");
  editButton->setIcon(QIcon(":/icons/edit.svg"));
  editButton->setIconSize(QSize(12, 12));
  editButton->setAutoRaise(true);

  auto *deleteButton = new QToolButton(this);
  deleteButton->setObjectName("IconButton");
  deleteButton->setIcon(QIcon(":/icons/trash.svg"));
  deleteButton->setIconSize(QSize(12, 12));
  deleteButton->setAutoRaise(true);

  actionsLayout->addWidget(pinButton);
  actionsLayout->addWidget(editButton);
  actionsLayout->addWidget(deleteButton);

  auto *actionsWidget = new QWidget(this);
  actionsWidget->setLayout(actionsLayout);

  layout->addLayout(textLayout, 1);
  layout->addWidget(actionsWidget, 0, Qt::AlignCenter);

  connect(pinButton, &QToolButton::clicked, this, &NoteItemWidget::pinClicked);
  connect(editButton, &QToolButton::clicked, this, &NoteItemWidget::editClicked);
  connect(deleteButton, &QToolButton::clicked, this,
          &NoteItemWidget::deleteClicked);
}

QString NoteItemWidget::noteId() const {
  return id;
}
