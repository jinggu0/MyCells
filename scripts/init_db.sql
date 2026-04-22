PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS cell_profiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    uuid TEXT NOT NULL UNIQUE,
    name TEXT NOT NULL,
    model_type TEXT NOT NULL,
    lineage_id TEXT,
    generation INTEGER NOT NULL DEFAULT 0,
    status TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS cell_snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    cell_id INTEGER NOT NULL UNIQUE,
    mass REAL NOT NULL,
    volume REAL NOT NULL,
    surface_area REAL NOT NULL,
    internal_nutrient REAL NOT NULL,
    stored_nutrient REAL NOT NULL,
    energy REAL NOT NULL,
    health REAL NOT NULL,
    stress REAL NOT NULL,
    membrane_integrity REAL NOT NULL,
    age_seconds REAL NOT NULL,
    division_readiness REAL NOT NULL,
    dormancy_state REAL NOT NULL DEFAULT 0,
    alive INTEGER NOT NULL DEFAULT 1,
    growth_phase TEXT,
    cell_cycle_progress REAL NOT NULL DEFAULT 0,
    last_simulated_at TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    FOREIGN KEY (cell_id) REFERENCES cell_profiles(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS environment_snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    cell_id INTEGER NOT NULL UNIQUE,
    nutrient_density REAL NOT NULL,
    oxygen_level REAL NOT NULL,
    temperature REAL NOT NULL,
    ph REAL NOT NULL,
    osmolarity REAL NOT NULL,
    toxin_level REAL NOT NULL,
    diffusion_rate REAL NOT NULL,
    micronutrient_level REAL DEFAULT 1,
    oxidative_stress_field REAL DEFAULT 0,
    mechanical_stress REAL DEFAULT 0,
    resource_gradient REAL DEFAULT 0,
    last_simulated_at TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    FOREIGN KEY (cell_id) REFERENCES cell_profiles(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS simulation_sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_uuid TEXT NOT NULL UNIQUE,
    cell_id INTEGER NOT NULL,
    started_at TEXT NOT NULL,
    ended_at TEXT,
    resume_from TEXT,
    simulated_until TEXT,
    offline_elapsed_seconds REAL NOT NULL DEFAULT 0,
    notes TEXT,
    FOREIGN KEY (cell_id) REFERENCES cell_profiles(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS event_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    cell_id INTEGER NOT NULL,
    session_id INTEGER,
    event_type TEXT NOT NULL,
    event_severity TEXT NOT NULL,
    event_time TEXT NOT NULL,
    sim_time_seconds REAL,
    title TEXT NOT NULL,
    description TEXT,
    payload_json TEXT,
    mass_before REAL,
    mass_after REAL,
    energy_before REAL,
    energy_after REAL,
    health_before REAL,
    health_after REAL,
    stress_before REAL,
    stress_after REAL,
    created_at TEXT NOT NULL,
    FOREIGN KEY (cell_id) REFERENCES cell_profiles(id) ON DELETE CASCADE,
    FOREIGN KEY (session_id) REFERENCES simulation_sessions(id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS time_series_samples (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    cell_id INTEGER NOT NULL,
    sample_time TEXT NOT NULL,
    sim_time_seconds REAL,
    mass REAL NOT NULL,
    volume REAL NOT NULL,
    energy REAL NOT NULL,
    health REAL NOT NULL,
    stress REAL NOT NULL,
    internal_nutrient REAL NOT NULL,
    stored_nutrient REAL NOT NULL,
    membrane_integrity REAL NOT NULL,
    division_readiness REAL NOT NULL,
    nutrient_density REAL,
    oxygen_level REAL,
    temperature REAL,
    ph REAL,
    osmolarity REAL,
    toxin_level REAL,
    created_at TEXT NOT NULL,
    FOREIGN KEY (cell_id) REFERENCES cell_profiles(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS simulation_configs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    cell_id INTEGER NOT NULL,
    config_version TEXT NOT NULL,
    model_type TEXT NOT NULL,
    params_json TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    is_active INTEGER NOT NULL DEFAULT 1,
    FOREIGN KEY (cell_id) REFERENCES cell_profiles(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS user_actions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    cell_id INTEGER NOT NULL,
    session_id INTEGER,
    action_type TEXT NOT NULL,
    action_time TEXT NOT NULL,
    payload_json TEXT,
    created_at TEXT NOT NULL,
    FOREIGN KEY (cell_id) REFERENCES cell_profiles(id) ON DELETE CASCADE,
    FOREIGN KEY (session_id) REFERENCES simulation_sessions(id) ON DELETE SET NULL
);

CREATE INDEX IF NOT EXISTS idx_cell_profiles_uuid
    ON cell_profiles(uuid);

CREATE INDEX IF NOT EXISTS idx_event_logs_cell_id_event_time
    ON event_logs(cell_id, event_time DESC);

CREATE INDEX IF NOT EXISTS idx_time_series_samples_cell_id_sample_time
    ON time_series_samples(cell_id, sample_time DESC);

CREATE INDEX IF NOT EXISTS idx_simulation_sessions_cell_id_started_at
    ON simulation_sessions(cell_id, started_at DESC);

CREATE INDEX IF NOT EXISTS idx_user_actions_cell_id_action_time
    ON user_actions(cell_id, action_time DESC);