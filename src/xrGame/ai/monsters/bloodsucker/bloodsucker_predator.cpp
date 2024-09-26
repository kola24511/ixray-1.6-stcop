#include "stdafx.h"
#include "bloodsucker.h"
#include "bloodsucker_predator.h"

#include "../control_animation_base.h"
#include "../control_direction_base.h"

#include "ai_object_location.h"
#include "../ai_monster_squad.h"
#include "../ai_monster_squad_manager.h"

#include "../states/state_move_to_point.h"
#include "../states/state_look_point.h"
#include "../states/state_custom_action.h"
#include "../../../cover_point.h"
#include "../monster_cover_manager.h"
#include "../monster_home.h"

#include "../../../actor.h"
#include "../../../actor_memory.h"
#include "../../../visual_memory_manager.h"

CustomBloodsuckerStatePredator::CustomBloodsuckerStatePredator(CustomBloodsucker*object) : inherited(object)
{
	m_target_node = 0;
	m_time_start_camp = 0;

	m_pBloodsucker = smart_cast<CustomBloodsucker*>(object);

	add_state	(eStatePredator_MoveToCover,	new CStateMonsterMoveToPointEx(object));
	add_state	(eStatePredator_LookOpenPlace, new CStateMonsterLookToPoint(object));
	add_state	(eStatePredator_Camp, new CStateMonsterCustomAction(object));
}

CustomBloodsuckerStatePredator::~CustomBloodsuckerStatePredator()
{

}

void CustomBloodsuckerStatePredator::reinit()
{
	inherited::reinit	();
}

void CustomBloodsuckerStatePredator::initialize()
{
	inherited::initialize						();

	this->m_pBloodsucker->predator_start						();

	select_camp_point							();
}

void CustomBloodsuckerStatePredator::reselect_state()
{
	if (this->prev_substate == u32(-1)) {
		this->select_state(eStatePredator_MoveToCover);
		return;
	}
	
	if (this->prev_substate == eStatePredator_MoveToCover) {
		this->select_state(eStatePredator_LookOpenPlace);
		return;
	}

	if (this->prev_substate == eStatePredator_LookOpenPlace) {
		this->select_state(eStatePredator_Camp);
		return;
	}

	this->select_state(eStatePredator_Camp);
}

void CustomBloodsuckerStatePredator::finalize()
{
	inherited::finalize							();

	this->m_pBloodsucker->predator_stop						();
	this->m_pBloodsucker->predator_unfreeze					();

	CMonsterSquad *squad = monster_squad().get_squad(this->object);
	squad->unlock_cover(m_target_node);

}

void CustomBloodsuckerStatePredator::critical_finalize()
{
	inherited::critical_finalize				();

	this->m_pBloodsucker->predator_stop						();
	this->m_pBloodsucker->predator_unfreeze					();

	CMonsterSquad *squad = monster_squad().get_squad(this->object);
	squad->unlock_cover(m_target_node);
}

bool CustomBloodsuckerStatePredator::check_start_conditions()
{
	if (Actor()->memory().visual().visible_now(this->object)) return false;
	return true;
}

bool CustomBloodsuckerStatePredator::check_completion()
{
	if (this->object->HitMemory.get_last_hit_time() > this->time_state_started) return true;
	if (this->object->EnemyMan.get_enemy() && this->object->EnemyMan.see_enemy_now() && (this->object->Position().distance_to(this->object->EnemyMan.get_enemy()->Position()) < 4.f)) return true;

	return false;
}

void CustomBloodsuckerStatePredator::setup_substates()
{
	state_ptr state = this->get_state_current();
	
	if (this->current_substate == eStatePredator_Camp) {
		this->m_pBloodsucker->predator_freeze	();
		m_time_start_camp		= time();

	} else {
		this->m_pBloodsucker->predator_unfreeze();
	}

	if (this->current_substate == eStatePredator_MoveToCover) {
		SStateDataMoveToPointEx data;

		data.vertex				= m_target_node;
		data.point				= ai().level_graph().vertex_position(data.vertex);
		data.action.action		= ACT_RUN;
		data.action.time_out	= 0;		// do not use time out
		data.completion_dist	= 0.f;		// get exactly to the point
		data.time_to_rebuild	= 0;		// do not rebuild
		data.accelerated		= true;
		data.braking			= true;
		data.accel_type 		= eAT_Aggressive;
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = this->object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));
		return;
	}

	if (this->current_substate == eStatePredator_LookOpenPlace) {

		SStateDataLookToPoint	data;

		Fvector dir;
		this->object->CoverMan->less_cover_direction(dir);

		data.point.mad			(this->object->Position(),dir,10.f);
		data.action.action		= ACT_STAND_IDLE;
		data.action.time_out	= 2000;		
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = this->object->db().m_dwIdleSndDelay;
		data.face_delay			= 0;

		state->fill_data_with(&data, sizeof(SStateDataLookToPoint));
		return;
	}

	if (this->current_substate == eStatePredator_Camp) {
		
		SStateDataAction data;

		data.action		= ACT_STAND_IDLE;
		data.time_out	= 0;			// do not use time out
		data.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.sound_delay = this->object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataAction));

		return;
	}
}

void CustomBloodsuckerStatePredator::check_force_state()
{
	const int TIME_TO_RESELECT_CAMP = 15000;

	if ((this->current_substate == eStatePredator_Camp) && (m_time_start_camp + TIME_TO_RESELECT_CAMP < time())) {
		if (this->current_substate != u32(-1))
			this->get_state_current()->critical_finalize();

		this->prev_substate		= u32(-1);
		this->current_substate	= u32(-1);

		CMonsterSquad *squad = monster_squad().get_squad(this->object);
		squad->unlock_cover	(m_target_node);

		select_camp_point	();
	}
}

void CustomBloodsuckerStatePredator::select_camp_point()
{
	m_target_node = u32(-1);
	if (this->object->Home->has_home()) {
		m_target_node							= this->object->Home->get_place_in_cover();
		if (m_target_node == u32(-1)) {
			m_target_node						= this->object->Home->get_place();
		}
	} 

	if (m_target_node == u32(-1)) {
		const CCoverPoint	*point = this->object->CoverMan->find_cover(this->object->Position(),10.f,30.f);
		if (point) {
			m_target_node				= point->level_vertex_id	();
		} 
	}

	if (m_target_node == u32(-1)) 
		m_target_node = this->m_pBloodsucker->ai_location().level_vertex_id();


	CMonsterSquad *squad = monster_squad().get_squad(this->object);
	squad->lock_cover(m_target_node);
}