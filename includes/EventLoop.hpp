#pragma once

#include <stdint.h>

#include "Config.hpp"
#include "SocketManager.hpp"

class EventLoop
{
	   public:
		EventLoop(SocketManager &socketManager, const Config &config);
		~EventLoop();

		// Main event loop
		bool initialize();
		void run();
		void stop();
		void cleanup();

		// Event handling
		void handleNewConnection(int serverFd);
		void handleClientRead(int clientFd);
		void handleClientWrite(int clientFd);
		void handleClientError(int clientFd);

		// Timeout management
		void checkTimeouts();
		void setConnectionTimeout(int seconds);

		// Configuration
		bool isRunning() const;

	   private:
		SocketManager &socketManager;
		const Config &config;

		int epollFd;  // For Linux (epoll)
		bool running;
		int connectionTimeout;

		static const int DEFAULT_TIMEOUT = 30;
		static const int DEFAULT_MAX_EVENTS = 64;

		// Platform-specific event handling
		bool initializeEpoll();
		bool addToEpoll(int fd, uint32_t events);
		bool removeFromEpoll(int fd);
		bool modifyEpoll(int fd, uint32_t events);

		// Helper methods
		bool isServerSocket(int fd) const;
};
