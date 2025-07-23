#pragma once
#include "Config.hpp"
#include "EventLoop.hpp"
#include "RequestHandler.hpp"
#include "SocketManager.hpp"

class HttpServer
{
       public:
	HttpServer(Config &config);
	~HttpServer();

	int start();
	void stop();
	void cleanup();	 // Make cleanup public

       private:
	Config config;
	SocketManager socketManager;
	EventLoop eventLoop;
	RequestHandler requestHandler;

	bool running;

	void startServer(const Config::ServerConfig &server);
	bool initializeServers();
};
