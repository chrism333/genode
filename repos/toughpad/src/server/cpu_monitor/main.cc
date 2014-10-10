/* Genode includes */
#include <base/env.h>
#include <base/printf.h>
#include <base/rpc_server.h>
#include <base/stdint.h>

#include <cap_session/connection.h>
#include <cpu_session/connection.h>
#include <cpu_session/cpu_session.h>
#include <timer_session/connection.h>

int main(int, char **)
{
	using namespace Genode;

	PDBG("--- cpu_profiler service started ---");

	Cap_connection          cap;
	Cpu_connection          cpu;
	Timer::Connection       timer;
	

	while (1) { 
		timer.msleep(2000);
		cpu.print_thread_info();
	}
	return 0;
}