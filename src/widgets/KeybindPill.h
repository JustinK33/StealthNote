#pragma once

#include <QWidget>

class QLabel;
class QToolButton;

class KeybindPill : public QWidget {
  Q_OBJECT
public:
  explicit KeybindPill(QWidget *parent = nullptr);
  void setText(const QString &text);

signals:
  void editClicked();

private:
  QLabel *label = nullptr;
  QToolButton *editButton = nullptr;
};
