#include "loadingspinner.h"
#include "utils/colors.h"
#include <QPainter>
#include <QPen>

LoadingSpinner::LoadingSpinner(QWidget* parent)
    : QLabel(parent)
    , m_angle(0)
{
    setFixedSize(60, 60);
    setScaledContents(true);
    setAlignment(Qt::AlignCenter);
    
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &LoadingSpinner::rotate);
}

void LoadingSpinner::start() {
    m_timer->start(50);
    show();
}

void LoadingSpinner::stop() {
    m_timer->stop();
    hide();
}

void LoadingSpinner::rotate() {
    m_angle = (m_angle + 30) % 360;
    update();
}

void LoadingSpinner::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QPen pen(Colors::toQColor(Colors::ACCENT_BLUE), 4);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    
    // Draw arc
    QRect rect(10, 10, 40, 40);
    int startAngle = -m_angle * 16;
    int spanAngle = 270 * 16;
    painter.drawArc(rect, startAngle, spanAngle);
}
