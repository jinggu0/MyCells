#include "services/SimulationService.h"
#include "storage/Database.h"
#include "storage/EventLogRepository.h"
#include "storage/UserActionRepository.h"

#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    try
    {
        const std::string db_path = "mycells.db";
        const std::string init_sql_path = "scripts/init_db.sql";
        const std::string cell_uuid = "default-cell";

        acell::services::SimulationService sim(db_path);
        sim.initialize(init_sql_path, cell_uuid);

        sim.feed(0.5);
        sim.setTemperature(36.8);
        sim.setPh(7.1);
        sim.reduceToxin(0.2);
        sim.renameCell("Companion Cell");

        for(int i=0; i<10; i++)
        {
            sim.update(1.0);
        }
        sim.saveNow();

        std::cout << "=== MyCells CLI Dashboard ===\n";
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Name: " << sim.cell().name << "\n";
        std::cout << "Mass: " << sim.cell().mass << "\n";
        std::cout << "Energy: " << sim.cell().energy << "\n";
        std::cout << "Health: " << sim.cell().health << "\n";
        std::cout << "Stress: " << sim.cell().stress << "\n";
        std::cout << "Nutrient density: " << sim.environment().nutrient_density << "\n";
        std::cout << "Temperature: " << sim.environment().temperature << "\n";
        std::cout << "pH: " << sim.environment().ph << "\n\n";

        acell::storage::Database db(db_path);
        acell::storage::EventLogRepository event_repo(db);
        acell::storage::UserActionRepository action_repo(db);

        auto events = event_repo.loadRecent(sim.cellId(), 5);
        auto actions = action_repo.loadRecent(sim.cellId(), 5);

        std::cout << "[Recent Events]\n";
        for(const auto& ev : events)
        {
            std::cout << "- [" << ev.event_severity << "] "
                      << ev.event_type << " | "
                      << ev.title << " | "
                      << ev.event_time << "\n";
        }

        std::cout << "\n[Recent User Actions]\n";
        for(const auto& a : actions)
        {
            std::cout << "- " << a.action_type << " | "
                      << a.action_time << " | "
                      << a.payload_json << "\n";
        }

        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}