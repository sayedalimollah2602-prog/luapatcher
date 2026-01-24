#ifndef LOADINGSPINNER_H
#define LOADINGSPINNER_H

#include <QLabel>
#include <QTimer>

class LoadingSpinner : public QLabel {
    Q_OBJECT

public:
    explicit LoadingSpinner(QWidget* parent = nullptr);
    
    void start();
    void stop();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void rotate();

private:
    QTimer* m_timer;
    int m_angle;
};

#endif // LOADINGSPINNER_H
