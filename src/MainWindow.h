#pragma once

#include <QMainWindow>
#include <QVector>

#include "DataStore.h"
#include "HotkeyManager.h"

class QTextEdit;
class QLabel;
class QVBoxLayout;
class QClipboard;
class KeybindPill;
class QLineEdit;
class QComboBox;
class QSystemTrayIcon;
class QMenu;
class QPropertyAnimation;
class QFocusEvent;

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override = default;

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;

private slots:
  void addNoteFromInput();
  void onClipboardChanged();
  void toggleVisibility();
  void beginHotkeyCapture();
  void saveData();

private:
  void buildUi();
  void applyTheme();
  void refreshNotesUi();
  void refreshClipsUi();
  void loadData();
  void updateTagFilter();
  void createTray();
  void animateShow();
  void animateHide();
  void animateFadeIn(QWidget *widget, int durationMs);
  void setStatus(const QString &text);
  void sortNotes();

  DataStore store;
  QVector<Note> notes;
  QVector<ClipItem> clips;
  Hotkey hotkey;
  Settings settings;

  QTextEdit *noteInput = nullptr;
  QLabel *notesCountLabel = nullptr;
  QLabel *clipsCountLabel = nullptr;
  QLabel *statusLabel = nullptr;
  QWidget *topBar = nullptr;
  KeybindPill *keybindPill = nullptr;
  QLineEdit *searchInput = nullptr;
  QComboBox *tagFilter = nullptr;
  QVBoxLayout *notesListLayout = nullptr;
  QVBoxLayout *clipsListLayout = nullptr;
  QString editingId;
  QString lastAddedNoteId;
  QString lastAddedClipText;
  QClipboard *clipboard = nullptr;
  HotkeyManager *hotkeyManager = nullptr;
  QSystemTrayIcon *trayIcon = nullptr;
  QMenu *trayMenu = nullptr;
  QPropertyAnimation *opacityAnimation = nullptr;
  bool dragging = false;
  QPoint dragStart;
};
