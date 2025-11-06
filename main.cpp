#include <iostream>
#include <mutex>
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
    int served = 0;     // number of parties served
    int total_time = 0; // total time served
};

// Global simulation parameters
int g_instances;               // number of concurrent dungeon instances
int g_tanks, g_healers, g_dps; // available players
int g_t1, g_t2;                // min/max time to complete dungeon

// Shared state
std::vector<Instance> instances;
std::mutex state_mutex;
std::mutex print_mutex;

void print_event_with_status(int instance_id, const std::string &event)
{
    std::string snapshot;
    {
        std::scoped_lock lock(state_mutex);
        snapshot = "[Status] ";
        for (int i = 0; i < g_instances; ++i)
        {
            std::string inst_str = "I" + std::to_string(i) + ":" + status_to_string(instances[i].status);
            snapshot += pad(inst_str, 12);
        }
    }
    std::scoped_lock print_lock(print_mutex);
    std::cout << "[I" << instance_id << "] " << event << '\n';
    std::cout << snapshot << '\n';
}

auto can_form_party() -> bool
{
    return (g_tanks >= 1 && g_healers >= 1 && g_dps >= 3);
}

void instance_loop(int instance_id)
{
    while (true)
    {
        // Try to form a party
        bool should_exit = false;
        {
            std::scoped_lock lock(state_mutex);
            if (!can_form_party())
            {
                instances[instance_id].status = InstanceStatus::Empty;
                should_exit = true;
            }
            else
            {
                // Form party atomically
                g_tanks -= 1;
                g_healers -= 1;
                g_dps -= 3;
                instances[instance_id].status = InstanceStatus::Active;
            }
        }

        if (should_exit)
        {
            print_event_with_status(instance_id, "No more players available");
            break; // Exit if no more parties can be formed
        }

        print_event_with_status(instance_id, "Party formed");

        // Simulate dungeon run
        int duration = random_int(g_t1, g_t2);
        std::this_thread::sleep_for(std::chrono::seconds(duration));

        // Update instance stats
        {
            std::scoped_lock lock(state_mutex);
            instances[instance_id].served += 1;
            instances[instance_id].total_time += duration;
            instances[instance_id].status = InstanceStatus::Empty;
        }

        print_event_with_status(instance_id, "Dungeon completed (" + std::to_string(duration) + "s)");
    }
}

auto main(int argc, char *argv[]) -> int
{
    if (argc != 7)
    {
        std::cerr << "Usage: " << argv[0] << " <instances> <tanks> <healers> <dps> <t1> <t2>\n";
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

    // Validate dungeon time range
    if (g_t1 < 1 || g_t2 < 1 || g_t1 > g_t2)
    {
        std::cerr << "Error: Invalid time range. Need 1 <= t1 <= t2\n";
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

    {
        std::scoped_lock print_lock(print_mutex);
        std::cout << "=== Starting LFG Simulation ===\n"
                  << pad("Instances:", 15) << g_instances << "\n"
                  << pad("Players:", 15) << "Tanks = " << g_tanks
                  << ", Healers = " << g_healers
                  << ", DPS = " << g_dps << "\n"
                  << pad("Clear time:", 15) << "[" << g_t1 << "," << g_t2 << "] seconds\n"
                  << "================================\n\n";
    }

    // Launch instance threads
    std::vector<std::thread> instance_workers;
    instance_workers.reserve(g_instances);
    for (int i = 0; i < g_instances; ++i)
    {
        instance_workers.emplace_back(instance_loop, i);
    }

    // Wait for all instance threads to complete
    for (auto &worker : instance_workers)
    {
        worker.join();
    }

    // Final summary
    int total_served = 0, total_time = 0;
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
              << "\nRemaining players:\n"
              << "  Tanks: " << g_tanks << "\n"
              << "  Healers: " << g_healers << "\n"
              << "  DPS: " << g_dps << "\n"
              << "==========================\n";

    return 0;
}
