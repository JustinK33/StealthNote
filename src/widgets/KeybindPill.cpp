#include "KeybindPill.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QIcon>

KeybindPill::KeybindPill(QWidget *parent) : QWidget(parent) {
  setObjectName("KeybindPill");

  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(10, 4, 10, 4);
  layout->setSpacing(6);

  label = new QLabel("fn + Space", this);
  label->setObjectName("KeybindLabel");

  editButton = new QToolButton(this);
  editButton->setObjectName("IconButton");
  editButton->setIcon(QIcon(":/icons/edit.svg"));
  editButton->setIconSize(QSize(12, 12));
  editButton->setAutoRaise(true);

  layout->addWidget(label);
  layout->addWidget(editButton);

  connect(editButton, &QToolButton::clicked, this, &KeybindPill::editClicked);
}

void KeybindPill::setText(const QString &text) {
  label->setText(text);
}
