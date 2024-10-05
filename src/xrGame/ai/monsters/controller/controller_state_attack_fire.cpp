#include "StdAfx.h"
#include "controller.h"
#include "controller_state_attack_fire.h"

#include "controller_animation.h"
#include "controller_direction.h"

#define MIN_ENEMY_DISTANCE	10.f
#define STATE_MAX_TIME		3000
#define STATE_EXECUTE_DELAY	5000


CStateControlFire::CStateControlFire(CBaseMonster* obj) : inherited(obj) 
{
	m_pController = smart_cast<CController*>(obj);
}

void CStateControlFire::reinit()
{
	inherited::reinit();

	m_time_state_last_execute = 0;

}


void CStateControlFire::initialize()
{
	inherited::initialize();
	this->m_pController->set_psy_fire_delay_zero();
	this->m_time_started = time();
}


void CStateControlFire::execute()
{
	this->object->dir().face_target(this->object->EnemyMan.get_enemy());
	this->m_pController->custom_dir().head_look_point(get_head_position(const_cast<CEntityAlive*>(this->object->EnemyMan.get_enemy())));

	this->m_pController->custom_anim().set_body_state(CControllerAnimation::eTorsoIdle, CControllerAnimation::eLegsTypeSteal);
}


void CStateControlFire::finalize()
{
	inherited::finalize();
	this->m_pController->set_psy_fire_delay_default();
	m_time_state_last_execute = time();
}

void CStateControlFire::critical_finalize()
{
	inherited::critical_finalize();
	this->m_pController->set_psy_fire_delay_default();
	m_time_state_last_execute = time();
}



bool CStateControlFire::check_start_conditions()
{
	if (!this->object->EnemyMan.see_enemy_now()) return false;
	if (this->object->EnemyMan.get_enemy()->Position().distance_to(this->object->Position()) < MIN_ENEMY_DISTANCE) return false;
	if (m_time_state_last_execute + STATE_EXECUTE_DELAY > time()) return false;

	return true;
}


bool CStateControlFire::check_completion()
{
	if (!this->object->EnemyMan.see_enemy_now()) return true;
	if (this->object->HitMemory.is_hit()) return true;
	if (this->object->EnemyMan.get_enemy()->Position().distance_to(this->object->Position()) < MIN_ENEMY_DISTANCE) return true;
	if (this->m_time_started + STATE_MAX_TIME < time()) return true;

	return false;
}
