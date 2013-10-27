#include "state_machine.h"
#include <algorithm>

StateMachine::start_event_t StateMachine::start_event;

void StateMachine :: set_state(const state_t  new_state)
{ 
    std::clog << "\tChanging state from " << get_state_name();
    current_state = new_state; 
    std::clog << " to " << get_state_name() << std::endl;
}

StateMachine::StateMachine(std::string states)
{
    const auto new_end = std::remove(states.begin(), states.end(), ' ');
    states.erase( new_end - states.begin() );
    for (; states.size();)
    {
        const size_t pos = states.find(',');
        states_names.push_back(states.substr(0, pos));
        states.erase(0, pos!= states.npos? (pos + 1) : pos);     
    }
}