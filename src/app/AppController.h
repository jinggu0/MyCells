#pragma once

#include "../services/SimulationService.h"

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QString>

#include <deque>
#include <memory>

class AppController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString cellName READ cellName NOTIFY stateChanged)
    Q_PROPERTY(QString cellStatus READ cellStatus NOTIFY stateChanged)
    Q_PROPERTY(QString growthPhase READ growthPhase NOTIFY stateChanged)
    Q_PROPERTY(bool alive READ alive NOTIFY stateChanged)

    Q_PROPERTY(double mass READ mass NOTIFY stateChanged)
    Q_PROPERTY(double energy READ energy NOTIFY stateChanged)
    Q_PROPERTY(double health READ health NOTIFY stateChanged)
    Q_PROPERTY(double stress READ stress NOTIFY stateChanged)
    Q_PROPERTY(double divisionReadiness READ divisionReadiness NOTIFY stateChanged)

    Q_PROPERTY(double nutrientDensity READ nutrientDensity NOTIFY stateChanged)
    Q_PROPERTY(double temperature READ temperature NOTIFY stateChanged)
    Q_PROPERTY(double ph READ ph NOTIFY stateChanged)
    Q_PROPERTY(double toxinLevel READ toxinLevel NOTIFY stateChanged)

    Q_PROPERTY(QString lastSimulatedAt READ lastSimulatedAt NOTIFY stateChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

    Q_PROPERTY(QVariantList recentEvents READ recentEvents NOTIFY recentEventsChanged)
    Q_PROPERTY(QVariantList recentActions READ recentActions NOTIFY recentActionsChanged)

    Q_PROPERTY(QVariantList massSeries READ massSeries NOTIFY chartsChanged)
    Q_PROPERTY(QVariantList energySeries READ energySeries NOTIFY chartsChanged)
    Q_PROPERTY(QVariantList healthSeries READ healthSeries NOTIFY chartsChanged)
    Q_PROPERTY(QVariantList stressSeries READ stressSeries NOTIFY chartsChanged)

    Q_PROPERTY(int feedBurstSerial READ feedBurstSerial NOTIFY feedBurstChanged)

public:
    explicit AppController(QObject* parent = nullptr);

    Q_INVOKABLE void initialize(const QString& dbPath,
                                const QString& initSqlPath,
                                const QString& cellUuid = "default-cell");

    Q_INVOKABLE void tickOnce();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void toggleRunning();

    Q_INVOKABLE void feed(double amount);
    Q_INVOKABLE void setTemperature(double value);
    Q_INVOKABLE void setPh(double value);
    Q_INVOKABLE void reduceToxin(double amount);
    Q_INVOKABLE void renameCell(const QString& newName);
    Q_INVOKABLE void resetSimulation();

    Q_INVOKABLE void refresh();

    QString cellName() const;
    QString cellStatus() const;
    QString growthPhase() const;
    bool alive() const;

    double mass() const;
    double energy() const;
    double health() const;
    double stress() const;
    double divisionReadiness() const;

    double nutrientDensity() const;
    double temperature() const;
    double ph() const;
    double toxinLevel() const;

    QString lastSimulatedAt() const;
    bool running() const;
    QString errorMessage() const;

    QVariantList recentEvents() const;
    QVariantList recentActions() const;

    QVariantList massSeries() const;
    QVariantList energySeries() const;
    QVariantList healthSeries() const;
    QVariantList stressSeries() const;

    int feedBurstSerial() const;

signals:
    void stateChanged();
    void runningChanged();
    void errorMessageChanged();
    void recentEventsChanged();
    void recentActionsChanged();
    void chartsChanged();
    void feedBurstChanged();

private slots:
    void onTick();

private:
    struct SamplePoint
    {
        double t = 0.0;
        double mass = 0.0;
        double energy = 0.0;
        double health = 0.0;
        double stress = 0.0;
    };

    void setError(const QString& msg);
    void clearError();
    void syncLists();

    QVariantList buildRecentEvents() const;
    QVariantList buildRecentActions() const;

    void appendCurrentSample();
    void rebuildChartSeries();

private:
    std::unique_ptr<acell::services::SimulationService> sim_;
    QTimer timer_;
    bool initialized_ = false;
    QString error_message_;

    QVariantList recent_events_;
    QVariantList recent_actions_;

    std::deque<SamplePoint> samples_;
    QVariantList mass_series_;
    QVariantList energy_series_;
    QVariantList health_series_;
    QVariantList stress_series_;

    int feed_burst_serial_ = 0;
    int max_samples_ = 180;
};