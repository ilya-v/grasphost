#ifndef STATE_MACHINE__H
#define STATE_MACHINE__H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <memory>

class StateMachine
{
    friend struct ActionBase;

public:
    typedef int state_t;

    struct start_event_t {};    
    static start_event_t start_event;

    struct event_handler_not_found_ex { std::string info; };

    static void start() { set_state(0);  process_event(&start_event); }

    static void set_state(const state_t  new_state);

    template <typename TEvent>  static void process_event(const TEvent *e)
    {
        std::clog << "\nState " << get_state() << "\tEvent " << typeid(e).name() << std::endl;
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
            throw  event_handler_not_found_ex{ std::to_string(get_state()) + ": " + typeid(e).name() };
        }
    }

protected:
    typedef std::map <state_t, std::vector<ActionBase *>> actions_map_t;
    static actions_map_t  actions;
    static void register_action(const state_t state, ActionBase *action) { actions[state];  
    actions[state].push_back(action); }
    static state_t  get_state()  { return current_state;      }

private:
    static state_t current_state;
};


typedef StateMachine :: start_event_t  StateMachine_StartEvent;


struct ActionBase
{
    ActionBase(const StateMachine :: state_t state)   { StateMachine :: register_action(state, this); }
    virtual ~ActionBase() {}
};

template <typename TEvent> struct Action : public ActionBase
{
    typedef void (*handler_t) (const TEvent *);
    handler_t  handler;
    Action(const StateMachine::state_t state, const handler_t  handler) : ActionBase(state), handler(handler) {}
    void handle(const TEvent *e) { handler(e); }
};


#define ACTION_TYPE(state, TEvent, e)           Action__##state##__##TEvent##__##e
#define HANDLER_FUNC_NAME(state, TEvent, e)     Handler__##state##__##TEvent##__##e

#define ACTION(state, TEvent, e) \
    void HANDLER_FUNC_NAME(state, TEvent, e)(const TEvent *); \
struct ACTION_TYPE(state, TEvent, e) : public Action<TEvent>{ \
    ACTION_TYPE(state, TEvent, e)() : Action<TEvent>(state, HANDLER_FUNC_NAME(state, TEvent, e)) {} \
} action_##state##__##TEvent##__##e##; \
    void HANDLER_FUNC_NAME(state, TEvent, e)(const TEvent *e)


#define EVENT(callback, event_struct)  void callback (const struct event_struct *e) { \
    std::clog << "\nEvent " << typeid(e).name() << std::endl;  StateMachine::process_event(e);     }

#endif