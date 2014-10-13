/* Genode includes */
#include <base/env.h>
#include <base/printf.h>
#include <base/rpc_server.h>
#include <base/stdint.h>

#include <cap_session/connection.h>
#include <cpu_session/connection.h>
#include <cpu_session/cpu_session.h>
#include <timer_session/connection.h>
#include <root/component.h>

namespace Genode { class Profiler_cpu_session_component; }

class Genode::Profiler_cpu_session_component : public Rpc_object<Nova_cpu_session>
{  
	private:
	  
		Cpu_connection &_cpu;
		
	public:
	  
		Profiler_cpu_session_component( Cpu_connection &cpu) : _cpu(cpu) {};
		
		Thread_capability create_thread(Name const &name,
		                                addr_t utcb = 0) override {
			PINF("Proxy:");
			return _cpu.create_thread(name, utcb); }

		Ram_dataspace_capability utcb(Thread_capability thread) override {
			return _cpu.utcb(thread); }

		void kill_thread(Thread_capability thread) override {
			_cpu.kill_thread(thread); }

		int start(Thread_capability thread, addr_t ip, addr_t sp) override {
			return _cpu.start(thread, ip, sp); }

		void pause(Thread_capability thread) override {
			_cpu.pause(thread); }

		void resume(Thread_capability thread) override {
			_cpu.resume(thread); }

		void cancel_blocking(Thread_capability thread) override {
			_cpu.cancel_blocking(thread); }

		Thread_state state(Thread_capability thread) override {
			return _cpu.state(thread); }

		void state(Thread_capability thread, Thread_state const &state) override {
			_cpu.state(thread, state); }

		void exception_handler(Thread_capability         thread,
		                       Signal_context_capability handler) override {
			_cpu.exception_handler(thread, handler); }

		void single_step(Thread_capability thread, bool b) override {
			_cpu.single_step(thread, b); }

		Affinity::Space affinity_space() const override {
			return _cpu.affinity_space(); }

		void affinity(Thread_capability thread, Affinity::Location affinity) override {
			_cpu.affinity(thread, affinity); }

		Dataspace_capability trace_control() override {
			return _cpu.trace_control(); }

		unsigned trace_control_index(Thread_capability thread) override {
			return _cpu.trace_control_index(thread); }

		Dataspace_capability trace_buffer(Thread_capability thread) override {
			return _cpu.trace_buffer(thread); }

		Dataspace_capability trace_policy(Thread_capability thread) override {
			return _cpu.trace_policy(thread); }

};


int main(int, char **)
{
	using namespace Genode;

	PDBG("--- cpu_profiler service started ---");

	Cap_connection          cap;
	Cpu_connection          cpu;
	Timer::Connection       timer;
	
	/* initialize server entry point */
	enum { STACK_SIZE = 2*1024*sizeof(Genode::addr_t) };
	static Genode::Rpc_entrypoint ep(&cap, STACK_SIZE, "cpu_profiler_ep");
	
	Root_component<Profiler_cpu_session_component> root(&ep, Genode::env()->heap());
	env()->parent()->announce(ep.manage(&root));
	
	while (1) { 
		timer.msleep(2000);
	}
	return 0;
}