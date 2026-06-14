#pragma once

#include <QWidget>

#include "../DataStore.h"

class QLabel;

class NoteItemWidget : public QWidget {
  Q_OBJECT
public:
  explicit NoteItemWidget(const Note &note, QWidget *parent = nullptr);
  QString noteId() const;

signals:
  void pinClicked();
  void editClicked();
  void deleteClicked();

private:
  QString id;
  QLabel *titleLabel = nullptr;
};
