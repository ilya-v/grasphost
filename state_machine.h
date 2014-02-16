#ifndef STATE_MACHINE__H
#define STATE_MACHINE__H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <memory>

struct ActionBase;
template <typename TEvent> struct Action;

class StateMachine
{
    friend struct ActionBase;

public:
    typedef int     state_t;

    static  struct  start_event_t               {}                              start_event;
            struct  event_handler_not_found_ex  { std::string info; };

                            StateMachine    (std::string states);              
                            StateMachine    (StateMachine &)        = delete;
            void            start           ()                                  { set_state(0);  process_event(&start_event); }
            state_t         get_state       ()                      const       { return current_state; }
            void            set_state       (const state_t state);
    const   std::string &   get_state_name  ()                      const       { return states_names[current_state]; }

    template <typename TEvent>  void process_event(const TEvent *e)
    {
        std::clog << "State " << get_state_name() << "\tEvent " << typeid(*e).name() << std::endl;
        bool handler_found = false;
        for (auto p_action : actions[current_state])
        {
            if (auto p_action_casted = dynamic_cast<Action<TEvent> *>(p_action))
            {
                std::clog << "\tHandler " << typeid(*p_action_casted).name() << std::endl;
                p_action_casted->handle(e);
                handler_found = true;
            }
        }
        if (!handler_found)
        {
            std::clog << "\tNo handler found for event" << std::endl;
            throw  event_handler_not_found_ex{ get_state_name() + ": " + typeid(e).name() };
        }
    }

private:
    typedef std::map <state_t, std::vector<ActionBase *>> actions_map_t;

    actions_map_t               actions;    
    state_t                     current_state = 0;
    std::vector<std::string>    states_names;

    void register_action(const state_t state, ActionBase *action) { actions[state].push_back(action); }
};


typedef StateMachine :: start_event_t  StateMachine_StartEvent;


struct ActionBase
{
    ActionBase(StateMachine &sm, const StateMachine :: state_t state) { sm.register_action(state, this); }
    virtual ~ActionBase() {}
};

template <typename TEvent> struct Action : public ActionBase
{
    typedef void (*handler_t) (const TEvent *);
    handler_t  handler;
    Action(StateMachine &sm, const StateMachine::state_t state, const handler_t  handler) : ActionBase(sm, state), handler(handler) {}
    void handle(const TEvent *e) { handler(e); }
};


#define SM_ACTION_TYPE(state, TEvent, e)           Action__##state##__##TEvent##__##e
#define SM_HANDLER_FUNC_NAME(state, TEvent, e)     Handler__##state##__##TEvent##__##e

#define SM_ACTION(state_machine, state, TEvent, e) \
    void SM_HANDLER_FUNC_NAME(state, TEvent, e)(const TEvent *); \
struct SM_ACTION_TYPE(state, TEvent, e) : public Action<TEvent>{ \
    SM_ACTION_TYPE(state, TEvent, e)() : Action<TEvent>(state_machine, state, SM_HANDLER_FUNC_NAME(state, TEvent, e)) {} \
} action_##state##__##TEvent##__##e; \
    void SM_HANDLER_FUNC_NAME(state, TEvent, e)(const TEvent *e)


#define SM_EVENT(state_machine, callback, event_struct)  void callback (const struct event_struct *e) { \
    std::clog << "\n\nEvent " << typeid(*e).name() << std::endl;  state_machine.process_event(e);  }

#define SM_DEFINE_WITH_STATES(sm, ...) enum {__VA_ARGS__}; StateMachine sm(#__VA_ARGS__);

#endif