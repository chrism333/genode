/* Genode includes */
#include <base/printf.h>

#include <cpu_session/connection.h>
#include <cpu_session/client.h>
#include <timer_session/connection.h>

int main(int, char **)
{
	using namespace Genode;

	PDBG("--- fb_switcher service started ---");

	Cpu_connection cpu;
	Timer::Connection timer;

	
	while (1) { 
		cpu.print_thread_info();
		
		timer.msleep(500);
	}
		

	return 0;
}