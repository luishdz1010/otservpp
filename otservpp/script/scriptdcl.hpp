#ifndef OTSERVPP_SCRIPT_SCRIPTDCL_HPP_
#define OTSERVPP_SCRIPT_SCRIPTDCL_HPP_

#define OTSERVPP_NAMESPACE_SCRIPT otservpp::script
#define OTSERVPP_NAMESPACE_SCRIPT_BEGIN namespace otservpp{ namespace script{
#define OTSERVPP_NAMESPACE_SCRIPT_END   } }

struct lua_State;

OTSERVPP_NAMESPACE_SCRIPT_BEGIN

class Adapter;
class StaticAdapters;

class SchedulerAdapter;

class Enviroment;
class EventDispatcher;

OTSERVPP_NAMESPACE_SCRIPT_END

#endif // OTSERVPP_SCRIPT_SCRIPTDCL_HPP_
