#include "MainWindow.h"

#include "widgets/ClipItemWidget.h"
#include "widgets/KeybindPill.h"
#include "widgets/NoteItemWidget.h"

#include <QApplication>
#include <QAbstractAnimation>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QLabel>
#include <QMouseEvent>
#include <QMenu>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QTextEdit>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QUuid>

#include <QSet>

#include <algorithm>

namespace {
void clearLayout(QLayout *layout) {
  while (auto *item = layout->takeAt(0)) {
    if (item->widget()) {
      item->widget()->deleteLater();
    }
    delete item;
  }
}

Note createNote(const QString &body, bool pinned = false) {
  Note note;
  note.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  note.body = body;
  note.pinned = pinned;
  note.updated = QDateTime::currentDateTime();
  return note;
}

ClipItem createClip(const QString &text) {
  ClipItem clip;
  clip.text = text;
  clip.captured = QDateTime::currentDateTime();
  return clip;
}

QStringList extractTags(const QString &text) {
  QRegularExpression regex("#([A-Za-z0-9_-]+)");
  QRegularExpressionMatchIterator iterator = regex.globalMatch(text);
  QSet<QString> tags;

  while (iterator.hasNext()) {
    QRegularExpressionMatch match = iterator.next();
    tags.insert(match.captured(1).toLower());
  }

  QStringList list = tags.values();
  list.sort();
  return list;
}

bool noteHasTag(const Note &note, const QString &tag) {
  const auto tags = extractTags(note.body);
  return tags.contains(tag.toLower());
}
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("QuickDraft");
  resize(1120, 680);
  setMinimumSize(980, 620);
  setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
  setAttribute(Qt::WA_TranslucentBackground);

  loadData();
  buildUi();
  applyTheme();
  createTray();

  keybindPill->setText(hotkey.toDisplayString());

  clipboard = QGuiApplication::clipboard();
  connect(clipboard, &QClipboard::dataChanged, this,
          &MainWindow::onClipboardChanged);

  hotkeyManager = new HotkeyManager(this);
  hotkeyManager->setHotkey(hotkey);
  connect(hotkeyManager, &HotkeyManager::activated, this,
          &MainWindow::toggleVisibility);
  hotkeyManager->start();

  refreshNotesUi();
  refreshClipsUi();
  setStatus("Saved");
}

void MainWindow::buildUi() {
  auto *root = new QWidget(this);
  root->setObjectName("Window");
  setCentralWidget(root);

  auto *rootLayout = new QVBoxLayout(root);
  rootLayout->setContentsMargins(10, 10, 10, 10);
  rootLayout->setSpacing(0);

  auto *frame = new QFrame(root);
  frame->setObjectName("Frame");
  rootLayout->addWidget(frame);

  auto *frameLayout = new QVBoxLayout(frame);
  frameLayout->setContentsMargins(0, 0, 0, 0);
  frameLayout->setSpacing(0);

  topBar = new QWidget(frame);
  topBar->setObjectName("TopBar");
  topBar->setFixedHeight(50);
  topBar->installEventFilter(this);
  frameLayout->addWidget(topBar);

  auto *topLayout = new QHBoxLayout(topBar);
  topLayout->setContentsMargins(20, 0, 20, 0);
  topLayout->setSpacing(12);

  auto *dots = new QWidget(topBar);
  auto *dotsLayout = new QHBoxLayout(dots);
  dotsLayout->setContentsMargins(0, 0, 0, 0);
  dotsLayout->setSpacing(8);

  auto *dotRed = new QWidget(dots);
  dotRed->setObjectName("MacDotRed");
  dotRed->setFixedSize(12, 12);

  auto *dotYellow = new QWidget(dots);
  dotYellow->setObjectName("MacDotYellow");
  dotYellow->setFixedSize(12, 12);

  auto *dotGreen = new QWidget(dots);
  dotGreen->setObjectName("MacDotGreen");
  dotGreen->setFixedSize(12, 12);

  dotsLayout->addWidget(dotRed);
  dotsLayout->addWidget(dotYellow);
  dotsLayout->addWidget(dotGreen);

  keybindPill = new KeybindPill(topBar);
  keybindPill->setFixedHeight(26);
  connect(keybindPill, &KeybindPill::editClicked, this,
          &MainWindow::beginHotkeyCapture);

  auto *leftGroup = new QWidget(topBar);
  auto *leftGroupLayout = new QHBoxLayout(leftGroup);
  leftGroupLayout->setContentsMargins(0, 0, 0, 0);
  leftGroupLayout->setSpacing(10);

  auto *title = new QLabel("QuickDraft", topBar);
  title->setObjectName("Title");

  leftGroupLayout->addWidget(dots);
  leftGroupLayout->addWidget(title);

  auto *settingsButton = new QToolButton(topBar);
  settingsButton->setObjectName("TopIconButton");
  settingsButton->setIcon(QIcon(":/icons/gear.svg"));
  settingsButton->setIconSize(QSize(14, 14));
  settingsButton->setAutoRaise(true);
  settingsButton->setFixedSize(26, 26);

  topLayout->addWidget(leftGroup, 0, Qt::AlignLeft);
  topLayout->addStretch(1);
  topLayout->addWidget(keybindPill, 0, Qt::AlignCenter);
  topLayout->addStretch(1);
  topLayout->addWidget(settingsButton, 0, Qt::AlignRight);

  auto *content = new QWidget(frame);
  content->setObjectName("Content");
  frameLayout->addWidget(content, 1);

  auto *contentLayout = new QHBoxLayout(content);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(0);

  auto *leftColumn = new QWidget(content);
  leftColumn->setObjectName("LeftColumn");

  auto *rightColumn = new QWidget(content);
  rightColumn->setObjectName("RightColumn");

  contentLayout->addWidget(leftColumn, 3);
  contentLayout->addWidget(rightColumn, 2);

  auto *leftLayout = new QVBoxLayout(leftColumn);
  leftLayout->setContentsMargins(20, 20, 20, 16);
  leftLayout->setSpacing(12);

  auto *notesHeader = new QWidget(leftColumn);
  auto *notesHeaderLayout = new QHBoxLayout(notesHeader);
  notesHeaderLayout->setContentsMargins(0, 0, 0, 0);
  notesHeaderLayout->setSpacing(8);

  auto *notesLabel = new QLabel("Notes", notesHeader);
  notesLabel->setObjectName("SectionLabel");
  auto *notesIcon = new QLabel(notesHeader);
  notesIcon->setObjectName("SectionIcon");
  notesIcon->setPixmap(QIcon(":/icons/note.svg").pixmap(14, 14));
  notesIcon->setFixedSize(14, 14);
  notesIcon->setScaledContents(true);

  auto *notesLine = new QFrame(notesHeader);
  notesLine->setObjectName("HeaderLine");
  notesLine->setFrameShape(QFrame::HLine);
  notesLine->setFrameShadow(QFrame::Plain);

  notesHeaderLayout->addWidget(notesIcon);
  notesHeaderLayout->addWidget(notesLabel);
  notesHeaderLayout->addWidget(notesLine, 1);

  noteInput = new QTextEdit(leftColumn);
  noteInput->setObjectName("NoteInput");
  noteInput->setPlaceholderText(
      "Start typing your quick note... (markdown supported)");
  noteInput->setMinimumHeight(120);

  auto *card = new QFrame(leftColumn);
  card->setObjectName("Card");
  auto *cardLayout = new QVBoxLayout(card);
  cardLayout->setContentsMargins(16, 14, 16, 14);
  cardLayout->setSpacing(12);

  auto *recentHeader = new QWidget(card);
  auto *recentHeaderLayout = new QHBoxLayout(recentHeader);
  recentHeaderLayout->setContentsMargins(0, 0, 0, 0);
  recentHeaderLayout->setSpacing(8);

  auto *recentLabel = new QLabel("Recent Notes", recentHeader);
  recentLabel->setObjectName("SectionLabel");
  notesCountLabel = new QLabel("0 items", recentHeader);
  notesCountLabel->setObjectName("SectionValue");
  notesCountLabel->setVisible(false);

  auto *recentLine = new QFrame(recentHeader);
  recentLine->setObjectName("HeaderLine");
  recentLine->setFrameShape(QFrame::HLine);
  recentLine->setFrameShadow(QFrame::Plain);

  recentHeaderLayout->addWidget(recentLabel);
  recentHeaderLayout->addWidget(recentLine, 1);

  auto *notesListContainer = new QWidget(card);
  notesListLayout = new QVBoxLayout(notesListContainer);
  notesListLayout->setContentsMargins(0, 0, 0, 0);
  notesListLayout->setSpacing(8);

  auto *notesScroll = new QScrollArea(card);
  notesScroll->setWidgetResizable(true);
  notesScroll->setWidget(notesListContainer);

  cardLayout->addWidget(recentHeader);
  cardLayout->addWidget(notesScroll, 1);

  leftLayout->addWidget(notesHeader);
  leftLayout->addWidget(noteInput, 1);
  leftLayout->addWidget(card, 1);

  auto *rightLayout = new QVBoxLayout(rightColumn);
  rightLayout->setContentsMargins(20, 20, 20, 16);
  rightLayout->setSpacing(12);

  auto *clipsHeader = new QWidget(rightColumn);
  auto *clipsHeaderLayout = new QHBoxLayout(clipsHeader);
  clipsHeaderLayout->setContentsMargins(0, 0, 0, 0);
  clipsHeaderLayout->setSpacing(8);

  auto *clipsLabel = new QLabel("Clipboard History", clipsHeader);
  clipsLabel->setObjectName("SectionLabel");
  clipsCountLabel = new QLabel("Auto-capture", clipsHeader);
  clipsCountLabel->setObjectName("SectionValue");
  clipsCountLabel->setVisible(false);

  auto *clipsIcon = new QLabel(clipsHeader);
  clipsIcon->setObjectName("SectionIcon");
  clipsIcon->setPixmap(QIcon(":/icons/clipboard.svg").pixmap(14, 14));
  clipsIcon->setFixedSize(14, 14);
  clipsIcon->setScaledContents(true);

  auto *clipsLine = new QFrame(clipsHeader);
  clipsLine->setObjectName("HeaderLine");
  clipsLine->setFrameShape(QFrame::HLine);
  clipsLine->setFrameShadow(QFrame::Plain);

  clipsHeaderLayout->addWidget(clipsIcon);
  clipsHeaderLayout->addWidget(clipsLabel);
  clipsHeaderLayout->addWidget(clipsLine, 1);

  auto *clipsListContainer = new QWidget(rightColumn);
  clipsListLayout = new QVBoxLayout(clipsListContainer);
  clipsListLayout->setContentsMargins(0, 0, 0, 0);
  clipsListLayout->setSpacing(8);

  auto *clipsScroll = new QScrollArea(rightColumn);
  clipsScroll->setWidgetResizable(true);
  clipsScroll->setWidget(clipsListContainer);

  rightLayout->addWidget(clipsHeader);
  rightLayout->addWidget(clipsScroll, 1);

  auto *footer = new QWidget(frame);
  footer->setObjectName("Footer");
  footer->setFixedHeight(48);
  frameLayout->addWidget(footer);

  auto *footerLayout = new QHBoxLayout(footer);
  footerLayout->setContentsMargins(20, 0, 20, 0);
  footerLayout->setSpacing(12);

  auto *footerLeft = new QWidget(footer);
  auto *footerLeftLayout = new QHBoxLayout(footerLeft);
  footerLeftLayout->setContentsMargins(0, 0, 0, 0);
  footerLeftLayout->setSpacing(12);

  auto *addButton = new QToolButton(footerLeft);
  addButton->setObjectName("IconButton");
  addButton->setIcon(QIcon(":/icons/plus.svg"));
  addButton->setIconSize(QSize(14, 14));
  addButton->setAutoRaise(true);

  auto *fileButton = new QToolButton(footerLeft);
  fileButton->setObjectName("IconButton");
  fileButton->setIcon(QIcon(":/icons/file.svg"));
  fileButton->setIconSize(QSize(14, 14));
  fileButton->setAutoRaise(true);

  footerLeftLayout->addWidget(addButton);
  footerLeftLayout->addWidget(fileButton);

  auto *statusWidget = new QWidget(footer);
  auto *statusLayout = new QHBoxLayout(statusWidget);
  statusLayout->setContentsMargins(0, 0, 0, 0);
  statusLayout->setSpacing(6);

  auto *statusIcon = new QLabel(statusWidget);
  statusIcon->setObjectName("StatusIcon");
  statusIcon->setPixmap(QIcon(":/icons/check.svg").pixmap(12, 12));
  statusIcon->setFixedSize(12, 12);
  statusIcon->setScaledContents(true);

  statusLabel = new QLabel("Saved", footer);
  statusLabel->setObjectName("StatusLabel");

  statusLayout->addWidget(statusIcon);
  statusLayout->addWidget(statusLabel);

  auto *avatar = new QWidget(footer);
  avatar->setObjectName("Avatar");
  avatar->setFixedSize(34, 34);

  footerLayout->addWidget(footerLeft);
  footerLayout->addStretch(1);
  footerLayout->addWidget(statusWidget);
  footerLayout->addStretch(1);
  footerLayout->addWidget(avatar, 0, Qt::AlignRight);

  connect(addButton, &QToolButton::clicked, this,
          &MainWindow::addNoteFromInput);

  connect(fileButton, &QToolButton::clicked, this, [this]() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(store.dataPath()));
  });

  auto *addShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
  connect(addShortcut, &QShortcut::activated, this,
          &MainWindow::addNoteFromInput);

  auto *addShortcutMeta =
      new QShortcut(QKeySequence(Qt::META | Qt::Key_Return), this);
  connect(addShortcutMeta, &QShortcut::activated, this,
          &MainWindow::addNoteFromInput);
}

void MainWindow::applyTheme() {
  QFont baseFont("SF Pro Text", 12);
  QApplication::setFont(baseFont);

  const QString style =
      "QWidget { color: #f5f7fa; }"
      "QFrame#Frame {"
      "  background: #0c0e12;"
      "  border: 1px solid rgba(255,255,255,18);"
      "  border-radius: 18px;"
      "}"
      "QWidget#TopBar, QWidget#Footer {"
      "  background: rgba(9,10,12,230);"
      "}"
      "QWidget#TopBar { border-bottom: 1px solid rgba(255,255,255,12); }"
      "QWidget#Footer { border-top: 1px solid rgba(255,255,255,12); }"
      "QWidget#RightColumn {"
      "  background: rgba(12,13,16,210);"
      "  border-left: 1px solid rgba(255,255,255,10);"
      "}"
      "QTextEdit#NoteInput {"
      "  background: rgba(8,9,11,210);"
      "  border: 1px solid rgba(255,255,255,14);"
      "  border-radius: 12px;"
      "  padding: 10px;"
      "  color: #e7eaee;"
      "}"
      "QFrame#Card {"
      "  background: transparent;"
      "  border: 1px solid rgba(255,255,255,10);"
      "  border-radius: 12px;"
      "}"
      "QWidget#NoteItem {"
      "  background: rgba(11,12,14,200);"
      "  border: 1px solid rgba(255,255,255,12);"
      "  border-radius: 10px;"
      "}"
      "QWidget#ClipItem {"
      "  background: rgba(11,12,14,200);"
      "  border: 1px solid rgba(255,255,255,12);"
      "  border-radius: 10px;"
      "}"
      "QToolButton#IconButton {"
      "  background: transparent;"
      "  border: none;"
      "  padding: 2px;"
      "}"
      "QToolButton#IconButton:hover {"
      "  background: rgba(255,255,255,12);"
      "  border-radius: 6px;"
      "}"
      "QToolButton#TopIconButton {"
      "  background: rgba(255,255,255,10);"
      "  border: 1px solid rgba(255,255,255,18);"
      "  border-radius: 8px;"
      "  padding: 4px;"
      "}"
      "QPushButton#CopyButton {"
      "  background: rgba(255,255,255,6);"
      "  border: 1px solid rgba(255,255,255,18);"
      "  border-radius: 8px;"
      "  padding: 4px 10px;"
      "  color: #e7eaee;"
      "  font-size: 10px;"
      "}"
      "QWidget#KeybindPill {"
      "  background: rgba(255,255,255,8);"
      "  border: 1px solid rgba(255,255,255,18);"
      "  border-radius: 999px;"
      "}"
      "QLabel#KeybindLabel { font-family: 'Menlo'; font-size: 11px; }"
      "QLabel#Title { color: #f1f3f6; font-size: 13px; }"
      "QLabel#SectionLabel { color: #cfd2d8; font-size: 11px; }"
      "QLabel#SectionValue { color: #8f98a6; font-size: 10px; }"
      "QLabel#NoteTitle { color: #e5e8ee; font-size: 12px; }"
      "QLabel#ClipText { color: #c7cbd2; font-size: 11px; }"
      "QLabel#StatusLabel { color: #9aa2ad; font-size: 11px; }"
      "QFrame#HeaderLine {"
      "  background: rgba(255,255,255,18);"
      "  min-height: 1px;"
      "  max-height: 1px;"
      "  border: none;"
      "}"
      "QWidget#Avatar {"
      "  background: qradialgradient(cx:0.3, cy:0.3, radius:1, fx:0.3, fy:0.3, "
      "stop:0 rgba(255,255,255,70), stop:1 rgba(255,255,255,6));"
      "  border: 1px solid rgba(255,255,255,18);"
      "  border-radius: 17px;"
      "}"
      "QDialog#KeybindDialog {"
      "  background: #0b0d10;"
      "  border: 1px solid rgba(255,255,255,24);"
      "}"
      "QWidget#MacDotRed { background: #ff5f56; border-radius: 6px; }"
      "QWidget#MacDotYellow { background: #ffbd2e; border-radius: 6px; }"
      "QWidget#MacDotGreen { background: #27c93f; border-radius: 6px; }"
      "QScrollArea { background: transparent; border: none; }"
      "QScrollArea > QWidget > QWidget { background: transparent; }"
      "QScrollBar:vertical, QScrollBar:horizontal { width: 0px; height: 0px; }";

  qApp->setStyleSheet(style);
}

void MainWindow::loadData() {
  if (!store.load(notes, clips, hotkey, settings)) {
    hotkey = Hotkey::defaultHotkey();
    settings = Settings();

    notes.clear();
    clips.clear();

    store.save(notes, clips, hotkey, settings);
  }

  sortNotes();
}

void MainWindow::refreshNotesUi() {
  clearLayout(notesListLayout);

  updateTagFilter();

  const QString searchText =
      searchInput ? searchInput->text().trimmed() : QString();
  const QString selectedTag =
      tagFilter ? tagFilter->currentData().toString() : QString();
  int visibleCount = 0;
  bool consumedHighlight = false;

  for (const auto &note : notes) {
    if (!selectedTag.isEmpty() && !noteHasTag(note, selectedTag)) {
      continue;
    }
    if (!searchText.isEmpty() &&
        !note.body.contains(searchText, Qt::CaseInsensitive)) {
      continue;
    }

    auto *item = new NoteItemWidget(note, this);

    connect(item, &NoteItemWidget::pinClicked, this, [this, note]() {
      for (auto &entry : notes) {
        if (entry.id == note.id) {
          entry.pinned = !entry.pinned;
          entry.updated = QDateTime::currentDateTime();
          break;
        }
      }
      sortNotes();
      refreshNotesUi();
      saveData();
    });

    connect(item, &NoteItemWidget::editClicked, this, [this, note]() {
      editingId = note.id;
      noteInput->setPlainText(note.body);
      noteInput->setFocus();
    });

    connect(item, &NoteItemWidget::deleteClicked, this, [this, note]() {
      notes.erase(std::remove_if(notes.begin(), notes.end(),
                                 [&note](const Note &entry) {
                                   return entry.id == note.id;
                                 }),
                  notes.end());
      refreshNotesUi();
      saveData();
    });

    notesListLayout->addWidget(item);
    visibleCount++;

    if (!consumedHighlight && note.id == lastAddedNoteId) {
      animateFadeIn(item, 240);
      consumedHighlight = true;
    }
  }

  notesListLayout->addStretch(1);
  if (visibleCount != notes.size()) {
    notesCountLabel->setText(
        QString("%1 of %2").arg(visibleCount).arg(notes.size()));
  } else {
    notesCountLabel->setText(QString("%1 items").arg(notes.size()));
  }
  if (consumedHighlight) {
    lastAddedNoteId.clear();
  }
}

void MainWindow::refreshClipsUi() {
  clearLayout(clipsListLayout);
  bool consumedHighlight = false;

  for (const auto &clip : clips) {
    auto *item = new ClipItemWidget(clip, this);
    connect(item, &ClipItemWidget::copyClicked, this,
            [this](const QString &text) {
              clipboard->setText(text);
            });
    clipsListLayout->addWidget(item);

    if (!consumedHighlight && clip.text == lastAddedClipText) {
      animateFadeIn(item, 220);
      consumedHighlight = true;
    }
  }

  clipsListLayout->addStretch(1);
  clipsCountLabel->setText(QString("%1 items").arg(clips.size()));
  if (consumedHighlight) {
    lastAddedClipText.clear();
  }
}

void MainWindow::addNoteFromInput() {
  const QString body = noteInput->toPlainText().trimmed();
  if (body.isEmpty()) {
    return;
  }

  if (!editingId.isEmpty()) {
    for (auto &note : notes) {
      if (note.id == editingId) {
        note.body = body;
        if (settings.autoPinTags &&
            body.contains("#pin", Qt::CaseInsensitive)) {
          note.pinned = true;
        }
        note.updated = QDateTime::currentDateTime();
        break;
      }
    }
    editingId.clear();
  } else {
    Note created = createNote(body);
    if (settings.autoPinTags &&
        body.contains("#pin", Qt::CaseInsensitive)) {
      created.pinned = true;
    }
    lastAddedNoteId = created.id;
    notes.push_front(created);
  }

  noteInput->clear();
  sortNotes();
  refreshNotesUi();
  saveData();
}

void MainWindow::onClipboardChanged() {
  const QString text = clipboard->text().trimmed();
  if (text.isEmpty()) {
    return;
  }

  if (!clips.isEmpty() && clips.front().text == text) {
    return;
  }

  clips.push_front(createClip(text));
  if (clips.size() > 8) {
    clips.resize(8);
  }

  lastAddedClipText = text;

  refreshClipsUi();
  saveData();
}

void MainWindow::toggleVisibility() {
  if (isVisible()) {
    animateHide();
  } else {
    show();
    raise();
    activateWindow();
    noteInput->setFocus();
    animateShow();
  }
}

void MainWindow::beginHotkeyCapture() {
  auto *dialog = new QDialog(this);
  dialog->setObjectName("KeybindDialog");
  dialog->setWindowTitle("Set Hotkey");
  dialog->setModal(true);
  dialog->setFixedSize(360, 180);

  auto *layout = new QVBoxLayout(dialog);
  layout->setContentsMargins(20, 20, 20, 20);
  layout->setSpacing(12);

  auto *title = new QLabel("Press the new shortcut", dialog);
  title->setObjectName("SectionLabel");
  auto *hint = new QLabel("Waiting for input...", dialog);
  hint->setObjectName("SectionValue");

  auto *buttonsRow = new QHBoxLayout();
  auto *resetButton = new QPushButton("Reset to Default", dialog);
  auto *cancelButton = new QPushButton("Cancel", dialog);

  buttonsRow->addWidget(resetButton);
  buttonsRow->addStretch(1);
  buttonsRow->addWidget(cancelButton);

  layout->addWidget(title, 0, Qt::AlignCenter);
  layout->addWidget(hint, 0, Qt::AlignCenter);
  layout->addStretch(1);
  layout->addLayout(buttonsRow);

  connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);
  connect(resetButton, &QPushButton::clicked, this, [this, dialog]() {
    hotkey = Hotkey::defaultHotkey();
    keybindPill->setText(hotkey.toDisplayString());
    hotkeyManager->setHotkey(hotkey);
    saveData();
    dialog->accept();
  });

  connect(dialog, &QDialog::finished, this, [this](int) {
    hotkeyManager->captureNext(nullptr);
  });

  hotkeyManager->captureNext([this, dialog](const Hotkey &captured) {
    hotkey = captured;
    keybindPill->setText(hotkey.toDisplayString());
    hotkeyManager->setHotkey(hotkey);
    saveData();
    dialog->accept();
  });

  dialog->exec();
}

void MainWindow::saveData() {
  store.save(notes, clips, hotkey, settings);
  setStatus("Saved");
}

void MainWindow::setStatus(const QString &text) {
  statusLabel->setText(text);
}

void MainWindow::sortNotes() {
  std::sort(notes.begin(), notes.end(), [](const Note &a, const Note &b) {
    if (a.pinned != b.pinned) {
      return a.pinned > b.pinned;
    }
    return a.updated > b.updated;
  });
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
  if (obj == topBar) {
    if (event->type() == QEvent::MouseButtonPress) {
      auto *mouseEvent = static_cast<QMouseEvent *>(event);
      if (mouseEvent->button() == Qt::LeftButton) {
        dragging = true;
        dragStart = mouseEvent->globalPosition().toPoint() -
              frameGeometry().topLeft();
        return true;
      }
    } else if (event->type() == QEvent::MouseMove && dragging) {
      auto *mouseEvent = static_cast<QMouseEvent *>(event);
      move(mouseEvent->globalPosition().toPoint() - dragStart);
      return true;
    } else if (event->type() == QEvent::MouseButtonRelease) {
      dragging = false;
      return true;
    }
  }
  return QMainWindow::eventFilter(obj, event);
}

void MainWindow::focusOutEvent(QFocusEvent *event) {
  QMainWindow::focusOutEvent(event);
  if (settings.autoHide && !QApplication::activeModalWidget()) {
    animateHide();
  }
}

void MainWindow::updateTagFilter() {
  if (!tagFilter) {
    return;
  }

  const QString currentTag = tagFilter->currentData().toString();
  QSet<QString> tagSet;
  for (const auto &note : notes) {
    for (const auto &tag : extractTags(note.body)) {
      tagSet.insert(tag);
    }
  }

  QStringList tags = tagSet.values();
  tags.sort();

  tagFilter->blockSignals(true);
  tagFilter->clear();
  tagFilter->addItem("All Tags", "");
  for (const auto &tag : tags) {
    tagFilter->addItem(QString("#%1").arg(tag), tag);
  }
  int index = tagFilter->findData(currentTag);
  if (index < 0) {
    index = 0;
  }
  tagFilter->setCurrentIndex(index);
  tagFilter->blockSignals(false);
}

void MainWindow::createTray() {
  if (!QSystemTrayIcon::isSystemTrayAvailable()) {
    return;
  }

  trayMenu = new QMenu(this);
  auto *toggleAction = trayMenu->addAction("Show/Hide QuickDraft");
  auto *newNoteAction = trayMenu->addAction("New Note");
  trayMenu->addSeparator();
  auto *autoHideAction = trayMenu->addAction("Auto-hide on focus loss");
  autoHideAction->setCheckable(true);
  autoHideAction->setChecked(settings.autoHide);
  auto *autoPinAction = trayMenu->addAction("Auto-pin #pin tags");
  autoPinAction->setCheckable(true);
  autoPinAction->setChecked(settings.autoPinTags);
  trayMenu->addSeparator();
  auto *quitAction = trayMenu->addAction("Quit");

  trayIcon = new QSystemTrayIcon(QIcon(":/icons/tray.svg"), this);
  trayIcon->setToolTip("QuickDraft");
  trayIcon->setContextMenu(trayMenu);
  trayIcon->show();

  connect(toggleAction, &QAction::triggered, this,
          &MainWindow::toggleVisibility);
  connect(newNoteAction, &QAction::triggered, this, [this]() {
    show();
    raise();
    activateWindow();
    noteInput->setFocus();
    animateShow();
  });
  connect(autoHideAction, &QAction::toggled, this, [this](bool enabled) {
    settings.autoHide = enabled;
    saveData();
  });
  connect(autoPinAction, &QAction::toggled, this, [this](bool enabled) {
    settings.autoPinTags = enabled;
    saveData();
  });
  connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

  connect(trayIcon, &QSystemTrayIcon::activated, this,
          [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger) {
              toggleVisibility();
            }
          });
}

void MainWindow::animateShow() {
  if (opacityAnimation) {
    opacityAnimation->stop();
  }
  setWindowOpacity(0.0);
  opacityAnimation = new QPropertyAnimation(this, "windowOpacity");
  opacityAnimation->setDuration(220);
  opacityAnimation->setStartValue(0.0);
  opacityAnimation->setEndValue(1.0);
  opacityAnimation->setEasingCurve(QEasingCurve::OutCubic);
  opacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::animateHide() {
  if (!isVisible()) {
    return;
  }
  if (opacityAnimation) {
    opacityAnimation->stop();
  }
  opacityAnimation = new QPropertyAnimation(this, "windowOpacity");
  opacityAnimation->setDuration(180);
  opacityAnimation->setStartValue(windowOpacity());
  opacityAnimation->setEndValue(0.0);
  opacityAnimation->setEasingCurve(QEasingCurve::InCubic);
  connect(opacityAnimation, &QPropertyAnimation::finished, this, [this]() {
    hide();
    setWindowOpacity(1.0);
  });
  opacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::animateFadeIn(QWidget *widget, int durationMs) {
  auto *effect = new QGraphicsOpacityEffect(widget);
  widget->setGraphicsEffect(effect);
  auto *animation = new QPropertyAnimation(effect, "opacity", widget);
  animation->setDuration(durationMs);
  animation->setStartValue(0.0);
  animation->setEndValue(1.0);
  animation->setEasingCurve(QEasingCurve::OutCubic);
  animation->start(QAbstractAnimation::DeleteWhenStopped);
}
