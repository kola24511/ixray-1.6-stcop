#include "StdAfx.h"
#include "monster_state_rest.h"


#include "monster_state_rest_sleep.h"
#include "monster_state_rest_walk_graph.h"
#include "monster_state_rest_idle.h"
#include "monster_state_rest_fun.h"
#include "monster_state_squad_rest.h"
#include "monster_state_squad_rest_follow.h"
#include "state_move_to_restrictor.h"
#include "../ai_monster_squad.h"
#include "../ai_monster_squad_manager.h"
#include "../anomaly_detector.h"
#include "monster_state_home_point_rest.h"
#include "monster_state_smart_terrain_task.h"

#define TIME_DELAY_FUN	20000
#define TIME_IDLE		60000


CStateMonsterRest::CStateMonsterRest(CBaseMonster* obj) : inherited(obj)
{
    this->add_state(eStateRest_Sleep, new CStateMonsterRestSleep(obj));
    this->add_state(eStateRest_WalkGraphPoint, new CStateMonsterRestWalkGraph(obj));
    this->add_state(eStateRest_Idle, new CStateMonsterRestIdle(obj));
    this->add_state(eStateRest_Fun, new CStateMonsterRestFun(obj));
    this->add_state(eStateSquad_Rest, new CStateMonsterSquadRest(obj));
    this->add_state(eStateSquad_RestFollow, new CStateMonsterSquadRestFollow(obj));
    this->add_state(eStateCustomMoveToRestrictor, new CStateMonsterMoveToRestrictor(obj));
    this->add_state(eStateRest_MoveToHomePoint, new CStateMonsterRestMoveToHomePoint(obj));
    this->add_state(eStateSmartTerrainTask, new CStateMonsterSmartTerrainTask(obj));
}


CStateMonsterRest::~CStateMonsterRest()
{
}


void CStateMonsterRest::initialize()
{
	inherited::initialize();

	time_last_fun = 0;
	time_idle_selected = Random.randI(2) ? 0 : time();
	this->object->anomaly_detector().activate();
}


void CStateMonsterRest::finalize()
{
	inherited::finalize();

	this->object->anomaly_detector().deactivate();
}


void CStateMonsterRest::critical_finalize()
{
	inherited::critical_finalize();

	this->object->anomaly_detector().deactivate();
}


void CStateMonsterRest::execute()
{
	// check alife control
	bool captured_by_smart_terrain = false;

	if (this->prev_substate == eStateSmartTerrainTask) {
		if (!this->get_state(eStateSmartTerrainTask)->check_completion())
			captured_by_smart_terrain = true;
	}
	else if (this->get_state(eStateSmartTerrainTask)->check_start_conditions())
		captured_by_smart_terrain = true;

	if (captured_by_smart_terrain) this->select_state(eStateSmartTerrainTask);
	else {
		// check restrictions
		bool move_to_restrictor = false;

		if (this->prev_substate == eStateCustomMoveToRestrictor) {
			if (!this->get_state(eStateCustomMoveToRestrictor)->check_completion())
				move_to_restrictor = true;
		}
		else if (this->get_state(eStateCustomMoveToRestrictor)->check_start_conditions())
			move_to_restrictor = true;

		if (move_to_restrictor)this->select_state(eStateCustomMoveToRestrictor);
		else {
			// check home point
			bool move_to_home_point = false;

			if (this->prev_substate == eStateRest_MoveToHomePoint) {
				if (!this->get_state(eStateRest_MoveToHomePoint)->check_completion())
					move_to_home_point = true;
			}
			else if (this->get_state(eStateRest_MoveToHomePoint)->check_start_conditions())
				move_to_home_point = true;

			if (move_to_home_point) this->select_state(eStateRest_MoveToHomePoint);
			else {
				// check squad behaviour
				bool use_squad = false;

				if (monster_squad().get_squad(this->object)->GetCommand(this->object).type == SC_REST) {
					this->select_state(eStateSquad_Rest);
					use_squad = true;
				}
				else if (monster_squad().get_squad(this->object)->GetCommand(this->object).type == SC_FOLLOW) {
					this->select_state(eStateSquad_RestFollow);
					use_squad = true;
				}

				if (!use_squad) {
					if (time_idle_selected + TIME_IDLE > time()) 			this->select_state(eStateRest_Idle);
					else if (time_idle_selected + TIME_IDLE + TIME_IDLE / 2 > time()) 	this->select_state(eStateRest_WalkGraphPoint);
					else {
						time_idle_selected = time();
						this->select_state(eStateRest_Idle);
					}
				}
			}
		}
	}

	this->get_state_current()->execute();
	this->prev_substate = this->current_substate;
}
