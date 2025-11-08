#include <iostream>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include "utils.h"

// Enum for instance status
enum class InstanceStatus
{
    Empty,
    Active
};

// Helper function to convert InstanceStatus to string
auto status_to_string(InstanceStatus status) -> std::string
{
    switch (status)
    {
    case InstanceStatus::Empty:
        return "empty";
    case InstanceStatus::Active:
        return "active";
    default:
        return "unknown";
    }
}

// Structure to represent a dungeon instance
struct Instance
{
    InstanceStatus status = InstanceStatus::Empty;
    int served = 0;           // number of parties served
    long long total_time = 0; // total time served
};

// Global simulation parameters
int g_instances;               // number of concurrent dungeon instances
int g_tanks, g_healers, g_dps; // available players
int g_t1, g_t2;                // min/max time to complete dungeon
int g_bonus_duration;          // in seconds, 0 = infinite

// Shared state
std::vector<Instance> instances;
std::mutex state_mutex;
std::mutex print_mutex;

// Simulation control
std::condition_variable player_available_cv;
bool simulation_ended = false;
bool bonus_mode_active = false;

// Bonus player tracking
int g_bonus_tanks_added = 0;
int g_bonus_healers_added = 0;
int g_bonus_dps_added = 0;

auto can_form_party() -> bool
{
    return (g_tanks >= 1 && g_healers >= 1 && g_dps >= 3);
}

void instance_loop(int instance_id)
{
    while (true)
    {
        // Try to form a party
        std::string status_snapshot;

        {
            std::unique_lock lock(state_mutex);

            // If can't form party and not in bonus mode yet, activate it
            if (!can_form_party() && !bonus_mode_active)
            {
                bonus_mode_active = true;
                {
                    std::scoped_lock print_lock(print_mutex);
                    std::cout << "\n[SYSTEM] Initial players exhausted. Activating bonus player generation...\n\n";
                }
                // Wake up the player generator thread
                player_available_cv.notify_all();
            }

            // Wait until a party can be formed or simulation ends
            player_available_cv.wait(lock, []() -> bool
                                     { return can_form_party() || simulation_ended; });

            if (simulation_ended && !can_form_party())
            {
                instances[instance_id].status = InstanceStatus::Empty;
                break;
            }

            // Form party atomically
            g_tanks -= 1;
            g_healers -= 1;
            g_dps -= 3;
            instances[instance_id].status = InstanceStatus::Active;

            // Capture status snapshot while still holding the lock
            status_snapshot = "[Status] ";
            for (int i = 0; i < g_instances; ++i)
            {
                std::string inst_str = "I" + std::to_string(i) + ":" + status_to_string(instances[i].status);
                status_snapshot += pad(inst_str, 12);
            }
        }

        // Simulate dungeon run
        int duration = random_int(g_t1, g_t2);

        // Print atomically
        {
            std::scoped_lock print_lock(print_mutex);
            std::cout << "[I" << instance_id << "] Dungeon started (" << duration << "s)\n";
            std::cout << status_snapshot << '\n';
        }

        std::this_thread::sleep_for(std::chrono::seconds(duration));

        // Update instance stats
        {
            std::unique_lock lock(state_mutex);
            instances[instance_id].served += 1;
            instances[instance_id].total_time += duration;
            instances[instance_id].status = InstanceStatus::Empty;

            // Capture status snapshot
            status_snapshot = "[Status] ";
            for (int i = 0; i < g_instances; ++i)
            {
                std::string inst_str = "I" + std::to_string(i) + ":" + status_to_string(instances[i].status);
                status_snapshot += pad(inst_str, 12);
            }
        }

        // Print atomically
        {
            std::scoped_lock print_lock(print_mutex);
            std::cout << "[I" << instance_id << "] Dungeon completed (" << duration << "s)\n";
            std::cout << status_snapshot << '\n';
        }
    }
}

void player_generator_thread()
{
    // Wait until bonus mode is activated
    {
        std::unique_lock lock(state_mutex);
        player_available_cv.wait(lock, []() -> bool
                                 { return bonus_mode_active || simulation_ended; });
        if (simulation_ended)
            return;
    }

    // Configuration for player generation
    const int check_interval_ms = 500;         // check every 500 ms
    const double generation_probability = 0.3; // 30% chance to generate players each check

    // Balance: Tanks and healers are rarer than DPS
    const int min_tanks_per_wave = 0;
    const int max_tanks_per_wave = 2;

    const int min_healers_per_wave = 0;
    const int max_healers_per_wave = 2;

    const int min_dps_per_wave = 0;
    const int max_dps_per_wave = 5;

    auto start_time = std::chrono::steady_clock::now();

    while (true)
    {
        // Check if bonus duration has elapsed
        if (g_bonus_duration > 0)
        {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                                       current_time - start_time)
                                       .count();
            if (elapsed_seconds >= g_bonus_duration)
            {
                // Signal all threads to end
                {
                    std::unique_lock lock(state_mutex);
                    simulation_ended = true;
                }
                player_available_cv.notify_all();
                break;
            }
        }

        // Random chance to generate players
        double roll = static_cast<double>(random_int(0, 100)) / 100.0;
        if (roll < generation_probability)
        {
            int new_tanks = random_int(min_tanks_per_wave, max_tanks_per_wave);
            int new_healers = random_int(min_healers_per_wave, max_healers_per_wave);
            int new_dps = random_int(min_dps_per_wave, max_dps_per_wave);

            // Only add players if at least one is generated
            if (new_tanks > 0 || new_healers > 0 || new_dps > 0)
            {
                {
                    std::scoped_lock lock(state_mutex);
                    g_tanks += new_tanks;
                    g_healers += new_healers;
                    g_dps += new_dps;

                    // Track bonus players added
                    g_bonus_tanks_added += new_tanks;
                    g_bonus_healers_added += new_healers;
                    g_bonus_dps_added += new_dps;
                }

                // Print notification
                {
                    std::scoped_lock print_lock(print_mutex);
                    std::cout << "[Player Generator] Added players - "
                              << "Tanks: " << new_tanks
                              << ", Healers: " << new_healers
                              << ", DPS: " << new_dps << "\n";
                }

                // Notify waiting instance threads
                player_available_cv.notify_all();
            }
        }

        // Sleep before next check
        std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms));
    }

    {
        std::scoped_lock print_lock(print_mutex);
        if (g_bonus_duration > 0)
        {
            std::cout << "\n[SYSTEM] Bonus duration ended. Finishing remaining dungeons...\n\n";
        }
    }
}

auto main(int argc, char *argv[]) -> int
{
    if (argc != 7 && argc != 8)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <instances> <tanks> <healers> <dps> <t1> <t2>\n";
        std::cerr << "  bonus_duration: seconds to generate bonus players (0 = infinite, omit = infinite)\n";
        return 1;
    }

    // Parse command-line arguments
    try
    {
        g_instances = std::stoi(argv[1]);
        g_tanks = std::stoi(argv[2]);
        g_healers = std::stoi(argv[3]);
        g_dps = std::stoi(argv[4]);
        g_t1 = std::stoi(argv[5]);
        g_t2 = std::stoi(argv[6]);

        if (argc == 8)
        {
            g_bonus_duration = std::stoi(argv[7]);
        }
        else
        {
            g_bonus_duration = 0; // infinite
        }
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << "Error: All arguments must be valid integers\n";
        return 1;
    }
    catch (const std::out_of_range &e)
    {
        std::cerr << "Error: Argument value too large\n";
        return 1;
    }

    // Validate all parameters are non-negative
    if (g_instances < 1 || g_tanks < 0 || g_healers < 0 || g_dps < 0)
    {
        std::cerr << "Error: Instances must be >= 1 and players must be >= 0\n";
        return 1;
    }

    constexpr int MAX_INSTANCES = 100;
    if (g_instances > MAX_INSTANCES)
    {
        std::cerr << "Error: Too many instances (max: " << MAX_INSTANCES << ")\n";
        return 1;
    }

    constexpr int MAX_PLAYERS = 10000;
    if (g_tanks > MAX_PLAYERS || g_healers > MAX_PLAYERS || g_dps > MAX_PLAYERS)
    {
        std::cerr << "Error: Player count exceeds maximum (" << MAX_PLAYERS << ")\n";
        return 1;
    }

    // Validate dungeon time range
    if (g_t1 < 1 || g_t2 < 1 || g_t1 > g_t2)
    {
        std::cerr << "Error: Invalid time range. Need 1 <= t1 <= t2\n";
        return 1;
    }

    // Validate bonus duration
    if (g_bonus_duration < 0)
    {
        std::cerr << "Error: bonus_duration must be >= 0 (0 = infinite)\n";
        return 1;
    }

    // Clamp times to valid range
    int original_t2 = g_t2;
    int original_t1 = g_t1;

    g_t2 = std::clamp(g_t2, 1, 15);
    g_t1 = std::clamp(g_t1, 1, g_t2);

    if (g_t1 != original_t1)
    {
        std::cout << "Note: t1 clamped from " << original_t1 << " to " << g_t1 << "\n";
    }
    if (g_t2 != original_t2)
    {
        std::cout << "Note: t2 clamped from " << original_t2 << " to " << g_t2 << " (max: 15)\n";
    }

    // Initialize dungeon instances
    instances.assign(g_instances, Instance{});

    if (!can_form_party())
    {
        std::cout << "Warning: Not enough players to form even one party (need 1 Tank, 1 Healer, 3 DPS)\n";
    }

    {
        std::scoped_lock print_lock(print_mutex);
        std::cout << "=== Starting LFG Simulation ===\n"
                  << pad("Instances:", 15) << g_instances << "\n"
                  << pad("Players:", 15) << "Tanks = " << g_tanks
                  << ", Healers = " << g_healers
                  << ", DPS = " << g_dps << "\n"
                  << pad("Clear time:", 15) << "[" << g_t1 << "," << g_t2 << "] seconds\n"
                  << pad("Bonus mode:", 15)
                  << (g_bonus_duration == 0 ? "Infinite" : std::to_string(g_bonus_duration) + " seconds")
                  << "\n"
                  << "================================\n\n";
    }

    // Launch instance threads
    std::vector<std::thread> instance_workers;
    instance_workers.reserve(g_instances);
    for (int i = 0; i < g_instances; ++i)
    {
        instance_workers.emplace_back(instance_loop, i);
    }

    // Launch player generator thread
    std::thread player_gen(player_generator_thread);

    // Wait for all instance threads to complete
    for (auto &worker : instance_workers)
    {
        worker.join();
    }

    // If bonus mode was never activated or infinite mode, end simulation
    {
        std::scoped_lock lock(state_mutex);
        if (!simulation_ended)
        {
            simulation_ended = true;
            player_available_cv.notify_all();
        }
    }

    // Wait for player generator to finish
    player_gen.join();

    // Final summary
    int total_served = 0;
    long long total_time = 0;
    std::cout << "\n=== Simulation Summary ===\n";
    for (int i = 0; i < g_instances; ++i)
    {
        const Instance &inst = instances[i];
        std::cout << "Instance " << i << ": Served " << inst.served
                  << " parties, Total time " << inst.total_time << " seconds\n";
        total_served += inst.served;
        total_time += inst.total_time;
    }
    std::cout << "--------------------------\n"
              << "Total parties served: " << total_served << "\n"
              << "Total time spent: " << total_time << " seconds\n"
              << "\nBonus players generated:\n"
              << "  Tanks: " << g_bonus_tanks_added << "\n"
              << "  Healers: " << g_bonus_healers_added << "\n"
              << "  DPS: " << g_bonus_dps_added << "\n"
              << "  Total: " << (g_bonus_tanks_added + g_bonus_healers_added + g_bonus_dps_added) << "\n"
              << "\nRemaining players:\n"
              << "  Tanks: " << g_tanks << "\n"
              << "  Healers: " << g_healers << "\n"
              << "  DPS: " << g_dps << "\n"
              << "==========================\n";

    return 0;
}
