#ifndef GENERATORWORKER_H
#define GENERATORWORKER_H

#include <QThread>
#include <QString>

class GeneratorWorker : public QThread {
    Q_OBJECT

public:
    explicit GeneratorWorker(const QString& appId, QObject* parent = nullptr);

signals:
    void progress(qint64 current, qint64 total);
    void status(QString message);
    void log(QString message, QString level = "INFO");
    void finished(QString path);
    void error(QString errorMessage);

protected:
    void run() override;

private:
    QString m_appId;
};

#endif // GENERATORWORKER_H
