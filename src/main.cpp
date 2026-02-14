#include "mainwindow.h"
#include "utils/colors.h"
#include <QApplication>
#include <QFont>

QString getStyleSheet() {
    return QString(R"(
QMainWindow {
    background: %1;
}

QWidget {
    font-family: 'Roboto', 'Segoe UI', sans-serif;
    color: %2;
}

/* Scrollbar */
QScrollBar:vertical {
    background-color: transparent;
    width: 6px;
    margin: 0px;
}
QScrollBar::handle:vertical {
    background-color: %3;
    border-radius: 3px;
    min-height: 20px;
}
QScrollBar::handle:vertical:hover {
    background-color: %4;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: none;
}

/* List Widget */
QListWidget {
    background-color: transparent;
    border: none;
    outline: none;
}
QListWidget::item {
    background-color: %5;
    border: 1px solid %3;
    border-radius: 16px;
    padding: 12px;
    margin-bottom: 8px;
    color: %2;
    word-wrap: break-word;
}
QListWidget::item:hover {
    background-color: %6;
    border: 1px solid %4;
}
QListWidget::item:selected {
    background-color: %7;
    border: 1px solid %8;
}

/* Inputs - Material Outlined Text Field */
QLineEdit {
    background-color: %5;
    border: 1px solid %3;
    border-radius: 28px;
    padding: 12px 20px;
    font-size: 14px;
    selection-background-color: %8;
    color: %2;
}
QLineEdit:focus {
    border: 2px solid %8;
    background-color: %6;
}
QLineEdit::placeholder {
    color: %4;
}

/* MessageBox - Material Dialog */
QMessageBox {
    background-color: %9;
    border-radius: 28px;
}
QMessageBox QLabel {
    color: %2;
    font-family: 'Roboto', 'Segoe UI', sans-serif;
}
QMessageBox QPushButton {
    background-color: %8;
    color: %10;
    border-radius: 20px;
    padding: 8px 24px;
    border: none;
    font-weight: bold;
    font-family: 'Roboto', 'Segoe UI', sans-serif;
}
QMessageBox QPushButton:hover {
    background-color: %11;
}

/* ToolTip */
QToolTip {
    background-color: %9;
    color: %2;
    border: 1px solid %3;
    border-radius: 8px;
    padding: 6px 12px;
    font-family: 'Roboto', 'Segoe UI', sans-serif;
}
)")
    .arg(Colors::SURFACE)                // %1  background
    .arg(Colors::ON_SURFACE)             // %2  text
    .arg(Colors::OUTLINE_VARIANT)        // %3  border/scrollbar
    .arg(Colors::OUTLINE)                // %4  hover border/scrollbar
    .arg(Colors::SURFACE_CONTAINER)      // %5  input/list bg
    .arg(Colors::SURFACE_CONTAINER_HIGH) // %6  hover bg
    .arg(Colors::SURFACE_CONTAINER_HIGHEST) // %7  selected bg
    .arg(Colors::PRIMARY)                // %8  primary accent
    .arg(Colors::SURFACE_CONTAINER_HIGH) // %9  dialog bg
    .arg(Colors::ON_PRIMARY)             // %10 on-primary text
    .arg(Colors::PRIMARY_CONTAINER);     // %11 primary hover
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon("logo.ico"));
    app.setStyle("Fusion");
    app.setStyleSheet(getStyleSheet());
    
    // Material Design font: Roboto (fallback Segoe UI)
    QFont font("Roboto");
    if (!QFontInfo(font).exactMatch()) {
#ifdef Q_OS_WIN
        font.setFamily("Segoe UI");
#else
        font.setFamily("sans-serif");
#endif
    }
    font.setPointSize(10);
    font.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(font);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
