#ifndef GAMECARD_H
#define GAMECARD_H

#include <QWidget>
#include <QPixmap>
#include <QMap>
#include <QTimer>

class GameCard : public QWidget {
    Q_OBJECT

public:
    explicit GameCard(QWidget* parent = nullptr);

    void setGameData(const QMap<QString, QString>& data);
    QMap<QString, QString> gameData() const;

    void setThumbnail(const QPixmap& pixmap);
    bool hasThumbnail() const;

    void setSelected(bool selected);
    bool isSelected() const;

    QString appId() const;

    void setSkeleton(bool skeleton);
    bool isSkeleton() const;

signals:
    void clicked(GameCard* card);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void updateSkeletonPulse();

private:
    QMap<QString, QString> m_data;
    QPixmap m_thumbnail;
    bool m_hasThumbnail = false;
    bool m_selected = false;
    bool m_hovered = false;
    bool m_isSkeleton = false;
    QTimer* m_skeletonTimer = nullptr;
    qreal m_skeletonPulse = 0.0;
    bool m_pulseIncreasing = true;
};

#endif // GAMECARD_H
