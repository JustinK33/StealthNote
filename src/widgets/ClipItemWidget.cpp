#include "ClipItemWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>

namespace {
QPushButton *makeCopyButton(QWidget *parent) {
  auto *button = new QPushButton(QStringLiteral("Copy Again"), parent);
  button->setObjectName(QStringLiteral("CopyButton"));
  button->setFixedHeight(22);
  return button;
}
} // namespace

ClipItemWidget::ClipItemWidget(const ClipItem &clip, QWidget *parent)
    : QWidget(parent), clipText(clip.text) {
  setObjectName(QStringLiteral("ClipItem"));

  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(12, 8, 12, 8);
  layout->setSpacing(10);

  textLabel = new QLabel(clip.text, this);
  textLabel->setObjectName(QStringLiteral("ClipText"));
  textLabel->setWordWrap(false);
  textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  copyButton = makeCopyButton(this);

  layout->addWidget(textLabel, 1);
  layout->addWidget(copyButton, 0, Qt::AlignRight);

  connect(copyButton, &QPushButton::clicked, this,
          [this, clipText = this->clipText](bool) {
            emit copyClicked(clipText);
          });
}

QString ClipItemWidget::text() const {
  return clipText;
}
