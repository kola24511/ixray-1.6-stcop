#include "StdAfx.h"

#include "../control_animation_base.h"
#include "../control_direction_base.h"

#include "ai_object_location.h"

#include "controller.h"

#include "controller_state_attack_moveout.h"

#include "controller_animation.h"
#include "controller_direction.h"

#define MAX_STATE_TIME						10000
#define DEFAULT_LOOK_POINT_CHANGE_DELAY		2000
#define LOOK_COVER_PROBABILITY				30

CStateControlMoveOut::CStateControlMoveOut(CBaseMonster* obj) : inherited(obj) 
{
	m_pController = smart_cast<CControllerBase*>(obj);
}

void CStateControlMoveOut::initialize()
{
	inherited::initialize();

	this->object->path().prepare_builder();
	m_last_time_look_point_updated = 0;

	m_state = eMoveToNodeEnemyLastSeen;

	m_current_delay = DEFAULT_LOOK_POINT_CHANGE_DELAY;

	// cheating here
	m_enemy_vertex = this->object->EnemyMan.get_enemy()->ai_location().level_vertex_id();
}

void CStateControlMoveOut::execute()
{

	update_target_point();

	this->object->path().set_target_point(m_target_position, m_target_node);
	this->object->path().set_rebuild_time(20000);
	this->object->path().set_distance_to_end(0.f);
	this->object->path().set_use_covers(false);


	this->object->anim().m_tAction = ACT_STEAL;
	this->object->anim().accel_deactivate();
	this->object->anim().accel_set_braking(false);
	this->object->set_state_sound(MonsterSound::eMonsterSoundAggressive);

	// look into very open point where we go
	update_look_point();
	this->m_pController->custom_dir().head_look_point(m_look_point);
	this->m_pController->custom_anim().set_body_state(CControllerAnimation::eTorsoSteal, CControllerAnimation::eLegsTypeStealMotion);
}

bool CStateControlMoveOut::check_start_conditions()
{
	if (this->object->EnemyMan.see_enemy_now())		return false;

	return true;
}

bool CStateControlMoveOut::check_completion()
{
	if (this->object->EnemyMan.see_enemy_now())		return true;
	if (this->time_state_started + 10000 < time())	return true;
	if (this->object->HitMemory.is_hit() &&
		this->object->HitMemory.get_last_hit_time() > this->time_state_started) return true;

	if (this->object->EnemyMan.get_enemy_position().distance_to(this->object->Position()) < 1.f) return true;

	return false;
}

void CStateControlMoveOut::update_target_point()
{
	if (m_state == eMoveToNodeEnemyLastSeen) {
		if (this->object->Position().similar(m_target_position, 0.05f)) m_state = eMoveToLastEnemyLocation;
	}

	if (m_state == eMoveToNodeEnemyLastSeen) {
		if (this->object->EnemyMan.get_my_vertex_enemy_last_seen() != u32(-1))
			m_target_node = this->object->EnemyMan.get_my_vertex_enemy_last_seen();
		else
			m_target_node = m_enemy_vertex;
		m_target_position = ai().level_graph().vertex_position(m_target_node);
	}
	else {
		m_target_node = m_enemy_vertex;
		m_target_position = ai().level_graph().vertex_position(m_target_node);
	}
}

void CStateControlMoveOut::update_look_point()
{
	if (this->object->HitMemory.get_last_hit_time() > this->object->EnemyMan.get_enemy_time_last_seen()) {
		m_look_point.mad(this->object->Position(), this->object->HitMemory.get_last_hit_dir(), 5.f);
		m_look_point.y += 1.5f;
		m_last_time_look_point_updated = time();
		return;
	}

	if (m_last_time_look_point_updated + m_current_delay > time()) return;

	if ((Random.randI(100) < LOOK_COVER_PROBABILITY) && (m_last_time_look_point_updated != 0)) {
		float angle = ai().level_graph().vertex_low_cover_angle(this->object->ai_location().level_vertex_id(), deg(10), std::greater<float>());
		m_look_point.mad(this->object->Position(), Fvector().setHP(angle, 0.f), 3.f);
		m_current_delay = DEFAULT_LOOK_POINT_CHANGE_DELAY;
	}
	else {
		m_look_point = this->object->EnemyMan.get_enemy_position();
		m_current_delay = DEFAULT_LOOK_POINT_CHANGE_DELAY * 2;
	}

	m_look_point.y += 1.5f;
	m_last_time_look_point_updated = time();
}
