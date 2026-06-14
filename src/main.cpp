#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("QuickDraft");
  app.setOrganizationName("QuickDraft");
  app.setOrganizationDomain("quickdraft.local");
  app.setQuitOnLastWindowClosed(false);

  MainWindow window;
  window.show();

  return app.exec();
}
