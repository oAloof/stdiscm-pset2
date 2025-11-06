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

void print_status_snapshot()
{
    std::string snapshot;
    {
        std::scoped_lock lock(state_mutex);
        snapshot = "[Status] ";
        for (int i = 0; i < g_instances; ++i)
        {
            snapshot += "I" + std::to_string(i) + ":" + status_to_string(instances[i].status);
            if (i + 1 < g_instances)
                snapshot += ' ';
        }
    }
    std::scoped_lock print_lock(print_mutex);
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

        print_status_snapshot();

        if (should_exit)
        {
            break; // Exit if no more parties can be formed
        }

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

        print_status_snapshot();
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

    return 0;
}
