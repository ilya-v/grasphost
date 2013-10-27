#include "state_machine.h"

StateMachine::start_event_t     StateMachine::start_event;

void StateMachine :: set_state(const state_t  new_state)
{ 
    std::clog << "\tChanging state from " << current_state << " to " << new_state << std::endl;
    current_state = new_state; 
}