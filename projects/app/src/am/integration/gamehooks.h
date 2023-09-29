#pragma once

#ifndef AM_STANDALONE

class GameHooks
{
public:
	// In some cases we have to install hooks from gta thread (currently we use render thread)
	// because calling some function will require operator new that requires allocator in tls and so on
	static void InitFromGtaThread();

	static void Init();
	static void Shutdown();
	static void EnableAll();
};

#endif
