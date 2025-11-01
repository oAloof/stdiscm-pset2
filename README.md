# STDISCM Problem Set 2: LFG Dungeon Queue Management

A C++ project demonstrating process synchronization through the implementation of a solution that manages the LFG (Looking for Group) dungeon queuing system for an MMORPG.

## 📋 Project Overview

This programming exercise assesses understanding of process synchronization. The solution must manage player queues and dungeon instances while ensuring proper synchronization to avoid deadlocks and starvation.

## 🎯 Specifications

The solution must satisfy the following requirements:

**a)** There are only `n` instances that can be concurrently active. Thus, there can only be a maximum `n` number of parties that are currently in a dungeon.

**b)** A standard party consists of **1 tank, 1 healer, 3 DPS**.

**c)** The solution should not result in a **deadlock**.

**d)** The solution should not result in **starvation**.

**e)** It is assumed that the input values arrive at the same time.

**f)** A time value `t` (in seconds) is randomly selected between `t1` and `t2`, where:
  - `t1` represents the fastest clear time of a dungeon instance
  - `t2` is the slowest clear time of a dungeon instance
  - For ease of testing: `t2 <= 15`

## 📥 Input

The program accepts the following inputs from the user:

| Input | Description                                              |
| ----- | -------------------------------------------------------- |
| `n`   | Maximum number of concurrent instances                   |
| `t`   | Number of tank players in the queue                      |
| `h`   | Number of healer players in the queue                    |
| `d`   | Number of DPS players in the queue                       |
| `t1`  | Minimum time before an instance is finished (in seconds) |
| `t2`  | Maximum time before an instance is finished (in seconds) |

## 📤 Output

The output of the program should show:

1. **Current status of all available instances**
   - If there is a party in the instance, the status should say "active"
   - If the instance is empty, the status should say "empty"

2. **Summary at the end of execution**
   - How many parties each instance has served
   - Total time served for each instance

## 🚀 Getting Started

### Prerequisites

- C++ compiler with C++20 support or higher
- CMake 3.16 or higher
- Git (for cloning the repository)

### Building the Project

1. **Clone the repository**:
```bash
git clone <repository-url>
cd stdiscm-pset2
```

2. **Configure with CMake** (run once, or after `CMakeLists.txt` changes):
```bash
cmake -S . -B build
```

3. **Build the project**:
```bash
cmake --build build
```

This will create the executable `pset2` in the `build/` directory.

### Rebuild After Changes

If you modify `CMakeLists.txt`:
```bash
cmake -S . -B build
cmake --build build
```

If you only modify source files:
```bash
cmake --build build
```

## 🏃 Running the Program

Run the program from the project root:

```bash
./build/pset2
```

Then follow the prompts to enter the required inputs:
- `n` (maximum concurrent instances)
- `t` (number of tanks)
- `h` (number of healers)
- `d` (number of DPS)
- `t1` (minimum dungeon clear time)
- `t2` (maximum dungeon clear time)

### Sample Output

The program displays:
- **Instance status**: Real-time status of all dungeon instances ("active" or "empty")
- **Summary**: Final statistics showing parties served and total time served per instance

## 📁 Project Structure

```
.
├── CMakeLists.txt                    # Build configuration
├── .clang-tidy                       # Linting configuration
├── .gitignore                        # Git ignore rules
├── README.md                         # This file
└── main.cpp                          # Main implementation
```

## 📦 Deliverables

1. Source code
2. Video Demonstration (test cases to be provided later)
3. Source code + build/compilation steps
4. Slides containing:
   - Possible deadlock and starvation explanation
   - Synchronization mechanisms used to solve the problem

## 👤 Author

**oAloof**
- GitHub: [@oAloof](https://github.com/oAloof)

---

*Problem Set 2 - AY4 Term 3 (SY2025-2026 Term 1) - STDISCM*
