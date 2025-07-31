#pragma once

#include <csignal>

// Global server pointer for signal handling
extern volatile sig_atomic_t shutdown_requested;
extern volatile sig_atomic_t received_signal;

// Signal handler function
void signalHandler(int signal);
