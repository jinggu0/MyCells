#include "AppController.h"

#include <QVariantMap>

AppController::AppController(QObject* parent)
    : QObject(parent)
{
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, &AppController::onTick);
}

void AppController::initialize(const QString& dbPath,
                               const QString& initSqlPath,
                               const QString& cellUuid)
{
    try
    {
        sim_ = std::make_unique<acell::services::SimulationService>(dbPath.toStdString());
        sim_->initialize(initSqlPath.toStdString(), cellUuid.toStdString());

        initialized_ = true;
        clearError();

        samples_.clear();
        appendCurrentSample();
        syncLists();

        emit stateChanged();
        resume();
    }
    catch(const std::exception& e)
    {
        setError(QString::fromUtf8(e.what()));
    }
}

void AppController::tickOnce()
{
    if(!initialized_ || !sim_)
    {
        return;
    }

    try
    {
        sim_->update(1.0);
        clearError();

        appendCurrentSample();
        syncLists();

        emit stateChanged();
    }
    catch(const std::exception& e)
    {
        setError(QString::fromUtf8(e.what()));
    }
}

void AppController::pause()
{
    if(timer_.isActive())
    {
        timer_.stop();
        emit runningChanged();
    }
}

void AppController::resume()
{
    if(!timer_.isActive())
    {
        timer_.start();
        emit runningChanged();
    }
}

void AppController::toggleRunning()
{
    if(timer_.isActive())
    {
        pause();
    }
    else
    {
        resume();
    }
}

void AppController::feed(double amount)
{
    if(!initialized_ || !sim_)
    {
        return;
    }

    try
    {
        sim_->feed(amount);
        clearError();

        feed_burst_serial_++;
        emit feedBurstChanged();

        appendCurrentSample();
        syncLists();

        emit stateChanged();
    }
    catch(const std::exception& e)
    {
        setError(QString::fromUtf8(e.what()));
    }
}

void AppController::setTemperature(double value)
{
    if(!initialized_ || !sim_)
    {
        return;
    }

    try
    {
        sim_->setTemperature(value);
        clearError();

        appendCurrentSample();
        syncLists();

        emit stateChanged();
    }
    catch(const std::exception& e)
    {
        setError(QString::fromUtf8(e.what()));
    }
}

void AppController::setPh(double value)
{
    if(!initialized_ || !sim_)
    {
        return;
    }

    try
    {
        sim_->setPh(value);
        clearError();

        appendCurrentSample();
        syncLists();

        emit stateChanged();
    }
    catch(const std::exception& e)
    {
        setError(QString::fromUtf8(e.what()));
    }
}

void AppController::reduceToxin(double amount)
{
    if(!initialized_ || !sim_)
    {
        return;
    }

    try
    {
        sim_->reduceToxin(amount);
        clearError();

        appendCurrentSample();
        syncLists();

        emit stateChanged();
    }
    catch(const std::exception& e)
    {
        setError(QString::fromUtf8(e.what()));
    }
}

void AppController::renameCell(const QString& newName)
{
    if(!initialized_ || !sim_)
    {
        return;
    }

    try
    {
        sim_->renameCell(newName.toStdString());
        clearError();

        syncLists();
        emit stateChanged();
    }
    catch(const std::exception& e)
    {
        setError(QString::fromUtf8(e.what()));
    }
}

void AppController::refresh()
{
    if(!initialized_ || !sim_)
    {
        return;
    }

    try
    {
        sim_->saveNow();
        clearError();

        appendCurrentSample();
        syncLists();

        emit stateChanged();
    }
    catch(const std::exception& e)
    {
        setError(QString::fromUtf8(e.what()));
    }
}

void AppController::resetSimulation()
{
    if(!initialized_ || !sim_)
    {
        return;
    }

    try
    {
        sim_->resetSimulation(true);
        clearError();

        samples_.clear();
        appendCurrentSample();
        syncLists();

        emit stateChanged();
    }
    catch(const std::exception& e)
    {
        setError(QString::fromUtf8(e.what()));
    }
}

QString AppController::cellName() const
{
    return (initialized_ && sim_) ? QString::fromStdString(sim_->cell().name) : QString();
}

QString AppController::cellStatus() const
{
    return (initialized_ && sim_)
        ? QString::fromStdString(acell::core::toString(sim_->cell().status))
        : QString();
}

QString AppController::growthPhase() const
{
    return (initialized_ && sim_)
        ? QString::fromStdString(acell::core::toString(sim_->cell().growth_phase))
        : QString();
}

bool AppController::alive() const
{
    return (initialized_ && sim_) ? sim_->cell().alive : false;
}

double AppController::mass() const
{
    return (initialized_ && sim_) ? sim_->cell().mass : 0.0;
}

double AppController::energy() const
{
    return (initialized_ && sim_) ? sim_->cell().energy : 0.0;
}

double AppController::health() const
{
    return (initialized_ && sim_) ? sim_->cell().health : 0.0;
}

double AppController::stress() const
{
    return (initialized_ && sim_) ? sim_->cell().stress : 0.0;
}

double AppController::divisionReadiness() const
{
    return (initialized_ && sim_) ? sim_->cell().division_readiness : 0.0;
}

double AppController::nutrientDensity() const
{
    return (initialized_ && sim_) ? sim_->environment().nutrient_density : 0.0;
}

double AppController::temperature() const
{
    return (initialized_ && sim_) ? sim_->environment().temperature : 0.0;
}

double AppController::ph() const
{
    return (initialized_ && sim_) ? sim_->environment().ph : 0.0;
}

double AppController::toxinLevel() const
{
    return (initialized_ && sim_) ? sim_->environment().toxin_level : 0.0;
}

QString AppController::lastSimulatedAt() const
{
    return (initialized_ && sim_) ? QString::fromStdString(sim_->lastSimulatedAt()) : QString();
}

bool AppController::running() const
{
    return timer_.isActive();
}

QString AppController::errorMessage() const
{
    return error_message_;
}

QVariantList AppController::recentEvents() const
{
    return recent_events_;
}

QVariantList AppController::recentActions() const
{
    return recent_actions_;
}

QVariantList AppController::massSeries() const
{
    return mass_series_;
}

QVariantList AppController::energySeries() const
{
    return energy_series_;
}

QVariantList AppController::healthSeries() const
{
    return health_series_;
}

QVariantList AppController::stressSeries() const
{
    return stress_series_;
}

int AppController::feedBurstSerial() const
{
    return feed_burst_serial_;
}

void AppController::onTick()
{
    tickOnce();
}

void AppController::setError(const QString& msg)
{
    if(error_message_ != msg)
    {
        error_message_ = msg;
        emit errorMessageChanged();
    }
}

void AppController::clearError()
{
    if(!error_message_.isEmpty())
    {
        error_message_.clear();
        emit errorMessageChanged();
    }
}

void AppController::syncLists()
{
    recent_events_ = buildRecentEvents();
    recent_actions_ = buildRecentActions();

    emit recentEventsChanged();
    emit recentActionsChanged();
}

QVariantList AppController::buildRecentEvents() const
{
    QVariantList list;

    if(!initialized_ || !sim_)
    {
        return list;
    }

    auto events = sim_->loadRecentEvents(10);
    for(const auto& ev : events)
    {
        QVariantMap m;
        m["type"] = QString::fromStdString(ev.event_type);
        m["severity"] = QString::fromStdString(ev.event_severity);
        m["title"] = QString::fromStdString(ev.title);
        m["description"] = QString::fromStdString(ev.description);
        m["time"] = QString::fromStdString(ev.event_time);
        list.push_back(m);
    }

    return list;
}

QVariantList AppController::buildRecentActions() const
{
    QVariantList list;

    if(!initialized_ || !sim_)
    {
        return list;
    }

    auto actions = sim_->loadRecentActions(10);
    for(const auto& a : actions)
    {
        QVariantMap m;
        m["type"] = QString::fromStdString(a.action_type);
        m["time"] = QString::fromStdString(a.action_time);
        m["payload"] = QString::fromStdString(a.payload_json);
        list.push_back(m);
    }

    return list;
}

void AppController::appendCurrentSample()
{
    if(!initialized_ || !sim_)
    {
        return;
    }

    SamplePoint p;
    p.t = sim_->cell().age_seconds;
    p.mass = sim_->cell().mass;
    p.energy = sim_->cell().energy;
    p.health = sim_->cell().health;
    p.stress = sim_->cell().stress;

    samples_.push_back(p);

    while(static_cast<int>(samples_.size()) > max_samples_)
    {
        samples_.pop_front();
    }

    rebuildChartSeries();
}

void AppController::rebuildChartSeries()
{
    mass_series_.clear();
    energy_series_.clear();
    health_series_.clear();
    stress_series_.clear();

    for(const auto& s : samples_)
    {
        QVariantMap m1;
        m1["x"] = s.t;
        m1["y"] = s.mass;
        mass_series_.push_back(m1);

        QVariantMap m2;
        m2["x"] = s.t;
        m2["y"] = s.energy;
        energy_series_.push_back(m2);

        QVariantMap m3;
        m3["x"] = s.t;
        m3["y"] = s.health;
        health_series_.push_back(m3);

        QVariantMap m4;
        m4["x"] = s.t;
        m4["y"] = s.stress;
        stress_series_.push_back(m4);
    }

    emit chartsChanged();
}