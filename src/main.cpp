#include "mainwindow.h"
#include "utils/colors.h"
#include <QApplication>
#include <QFont>

QString getStyleSheet() {
    return QString(R"(
QMainWindow {
    background: transparent;
}

QWidget {
    font-family: 'Segoe UI', sans-serif;
    color: %1;
}

/* Scrollbar */
QScrollBar:vertical {
    background-color: transparent;
    width: 6px;
    margin: 0px;
}
QScrollBar::handle:vertical {
    background-color: %2;
    border-radius: 3px;
    min-height: 20px;
}
QScrollBar::handle:vertical:hover {
    background-color: %3;
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
    background-color: %4;
    border: 1px solid %2;
    border-radius: 12px;
    padding: 12px;
    margin-bottom: 8px;
    color: %1;
    word-wrap: break-word;
}
QListWidget::item:hover {
    background-color: %5;
    border: 1px solid %6;
}
QListWidget::item:selected {
    background-color: %7;
    border: 1px solid %3;
}

/* Inputs */
QLineEdit {
    background-color: %4;
    border: 1px solid %2;
    border-radius: 12px;
    padding: 14px 16px;
    font-size: 14px;
    selection-background-color: %3;
    color: %1;
}
QLineEdit:focus {
    border: 1px solid %3;
    background-color: %5;
}

/* MessageBox */
QMessageBox {
    background-color: #1e293b;
}
QMessageBox QLabel {
    color: %1;
}
QMessageBox QPushButton {
    background-color: %3;
    color: white;
    border-radius: 6px;
    padding: 6px 16px;
    border: none;
}
)")
    .arg(Colors::TEXT_PRIMARY)       // %1
    .arg(Colors::GLASS_BORDER)       // %2
    .arg(Colors::ACCENT_BLUE)        // %3
    .arg(Colors::GLASS_BG)           // %4
    .arg(Colors::GLASS_HOVER)        // %5
    .arg(Colors::ACCENT_BLUE + "80") // %6
    .arg(Colors::ACCENT_BLUE + "20"); // %7
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon("logo.ico"));
    app.setStyle("Fusion");
    app.setStyleSheet(getStyleSheet());
    
    // Set global font
    QFont font = app.font();
#ifdef Q_OS_WIN
    font.setFamily("Segoe UI");
#endif
    font.setPointSize(10);
    app.setFont(font);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
