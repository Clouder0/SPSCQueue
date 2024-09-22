#pragma once

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iomanip>
#include <system_error>

#include <unistd.h>
#include <sched.h>
#include <sys/syscall.h>

class cpuset
{
public:
    cpuset()
    {
        clear();
    }

    cpuset(int cpu)
    {
        clear();
        insert(cpu);
    }

    cpuset(const cpuset &other)
    {
        copy_cpuset(other);
    }

    cpuset &operator=(const cpuset &other)
    {
        copy_cpuset(other);
        return *this;
    }

    void clear()
    {
        CPU_ZERO(&cpuset_);
    }

    void insert(int cpu)
    {
        CPU_SET(cpu, &cpuset_);
    }

    void remove(int cpu)
    {
        CPU_CLR(cpu, &cpuset_);
    }

    bool find(int cpu) const
    {
        return CPU_ISSET(cpu, &cpuset_);
    }

    std::size_t size() const
    {
        return CPU_COUNT(&cpuset_);
    }

    std::size_t max_cpus() const
    {
        return CPU_SETSIZE - 1;
    }

    friend void   set_cpu_affinity(const cpuset &cpuset);
    friend cpuset get_cpu_affinity();

    cpuset &operator&=(const cpuset &rhs)
    {
        CPU_AND(&cpuset_, &rhs.cpuset_, &cpuset_);
        return *this;
    }

    cpuset &operator|=(const cpuset &rhs)
    {
        CPU_OR(&cpuset_, &rhs.cpuset_, &cpuset_);
        return *this;
    }

    cpuset &operator^=(const cpuset &rhs)
    {
        CPU_XOR(&cpuset_, &rhs.cpuset_, &cpuset_);
        return *this;
    }

    friend bool   operator==(const cpuset &lhs, const cpuset &rhs);
    friend cpuset operator&(const cpuset &rhs, const cpuset &lhs);
    friend cpuset operator|(const cpuset &rhs, const cpuset &lhs);
    friend cpuset operator^(const cpuset &rhs, const cpuset &lhs);

private:
    void copy_cpuset(const cpuset &other)
    {
        std::memcpy(&cpuset_, &other.cpuset_, sizeof(cpu_set_t));
    }

    cpu_set_t cpuset_;
};

bool operator==(const cpuset &lhs, const cpuset &rhs)
{
    return CPU_EQUAL(&lhs.cpuset_, &rhs.cpuset_);
}

cpuset operator&(const cpuset &rhs, const cpuset &lhs)
{
    cpuset ret;
    CPU_AND(&ret.cpuset_, &rhs.cpuset_, &lhs.cpuset_);
    return ret;
}

cpuset operator|(const cpuset &rhs, const cpuset &lhs)
{
    cpuset ret;
    CPU_OR(&ret.cpuset_, &rhs.cpuset_, &lhs.cpuset_);
    return ret;
}

cpuset operator^(const cpuset &rhs, const cpuset &lhs)
{
    cpuset ret;
    CPU_XOR(&ret.cpuset_, &rhs.cpuset_, &lhs.cpuset_);
    return ret;
}

std::ostream &operator<<(std::ostream &os, const cpuset &cpuset)
{
    os << "[";
    for (std::size_t i = 0; i < cpuset.max_cpus(); ++i) {
        if (cpuset.find(i)) {
            os << std::setw(4) << i;
        }
    }

    os << "]";
    return os;
}

int get_current_cpu()
{
    return sched_getcpu();
}

void set_cpu_affinity(const cpuset &cpuset)
{
    auto err = sched_setaffinity(0, sizeof(cpuset.cpuset_), &cpuset.cpuset_);
    if (err < 0) {
        throw std::system_error(errno, std::system_category());
    }
}

cpuset get_cpu_affinity()
{
    cpuset ret;
    auto err = sched_getaffinity(0, sizeof(ret.cpuset_), &ret.cpuset_);
    if (err < 0) {
        throw std::system_error(errno, std::system_category());
    }

    return ret;
}