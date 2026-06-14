#pragma once

#include <QDateTime>
#include <QVector>
#include <QString>

#include "HotkeyManager.h"

struct Note {
  QString id;
  QString body;
  bool pinned = false;
  QDateTime updated;
};

struct ClipItem {
  QString text;
  QDateTime captured;
};

struct Settings {
  bool autoHide = true;
  bool autoPinTags = true;
};

class DataStore {
public:
  DataStore() = default;

  bool load(QVector<Note> &notes, QVector<ClipItem> &clips, Hotkey &hotkey,
            Settings &settings);
  bool save(const QVector<Note> &notes, const QVector<ClipItem> &clips,
            const Hotkey &hotkey, const Settings &settings) const;
  QString dataPath() const;
};
