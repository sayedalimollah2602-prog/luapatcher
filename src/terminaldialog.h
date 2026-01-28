#ifndef TERMINALDIALOG_H
#define TERMINALDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

class TerminalDialog : public QDialog {
    Q_OBJECT

public:
    explicit TerminalDialog(QWidget* parent = nullptr);
    
    void appendLog(const QString& message, const QString& level = "INFO");
    void clear();
    void setFinished(bool success);

private:
    QTextEdit* m_logView;
    QPushButton* m_closeBtn;
};

#endif // TERMINALDIALOG_H
